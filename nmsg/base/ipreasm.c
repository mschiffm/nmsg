/*
 * Copyright (c) 2010-2012 by Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * ipreasm -- Routines for reassembly of fragmented IPv4 and IPv6 packets.
 *
 * Copyright (c) 2007  Jan Andres <jandres@gmx.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "nmsg_port_net.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <pcap.h>

#include "ipreasm.h"

#define REASM_IP_HASH_SIZE 1021U

/*
 * This struct contains some metadata, the main hash table, and a pointer
 * to the first entry that will time out. A linked list is kept in the
 * order in which packets will time out. Using a linked list for this
 * purpose requires that packets are input in chronological order, and
 * that a constant timeout value is used, which doesn't change even when
 * the entry's state transitions from active to invalid.
 */
struct reasm_ip {
	struct reasm_ip_entry *table[REASM_IP_HASH_SIZE];
	struct reasm_ip_entry *time_first, *time_last;
	unsigned waiting, max_waiting, timed_out, dropped_frags;
	struct timespec timeout;
};

/*
 * Hash functions.
 */
static unsigned reasm_ipv4_hash(const struct reasm_id_ipv4 *id);
static unsigned reasm_ipv6_hash(const struct reasm_id_ipv6 *id);

static void remove_entry(struct reasm_ip *reasm, struct reasm_ip_entry *entry);

/*
 * Dispose of any entries which have expired before "now".
 */
static void process_timeouts(struct reasm_ip *reasm, const struct timespec *now);

/*
 * Create fragment structure from IPv6 packet. Returns NULL if the input
 * is not a fragment.
 * This function is called by parse_packet(), don't call it directly.
 */
static struct reasm_frag_entry *frag_from_ipv6(const uint8_t *packet,
					       uint32_t *ip_id, bool *last_frag);

/*
 * Compare packet identification tuples for specified protocol.
 */
static bool reasm_id_equal(enum reasm_proto proto,
			   const union reasm_id *left, const union reasm_id *right);

static unsigned
reasm_ipv4_hash(const struct reasm_id_ipv4 *id) {
	unsigned hash = 0;
	int i;

	for (i = 0; i < 4; i++) {
		hash = 37U * hash + id->ip_src[i];
		hash = 37U * hash + id->ip_dst[i];
	}

	hash = 59U * hash + id->ip_id;

	hash = 47U * hash + id->ip_proto;

	return (hash);
}

static unsigned
reasm_ipv6_hash(const struct reasm_id_ipv6 *id) {
	unsigned hash = 0;
	int i;

	for (i = 0; i < 16; i++) {
		hash = 37U * hash + id->ip_src[i];
		hash = 37U * hash + id->ip_dst[i];
	}

	hash = 59U * hash + id->ip_id;

	return (hash);
}

bool
reasm_ip_next(struct reasm_ip *reasm, const uint8_t *packet, unsigned len,
	      const struct timespec *timestamp, struct reasm_ip_entry **out_entry)
{
	enum reasm_proto proto;
	union reasm_id id;
	unsigned hash = 0;
	bool last_frag = 0;
	struct reasm_frag_entry *frag;
	struct reasm_ip_entry *entry;

	process_timeouts(reasm, timestamp);

	frag = reasm_parse_packet(packet, len, timestamp, &proto, &id, &hash, &last_frag);
	if (frag == NULL) {
		/* some packet that we don't recognize as a fragment */
		return (false);
	}

	hash %= REASM_IP_HASH_SIZE;
	entry = reasm->table[hash];
	while (entry != NULL &&
	       (proto != entry->protocol ||
		!reasm_id_equal(proto, &id, &entry->id)))
	{
		entry = entry->next;
	}

	if (entry == NULL) {
		struct reasm_frag_entry *list_head;

		entry = malloc(sizeof(*entry));
		if (entry == NULL) {
			free(frag);
			abort();
		}

		list_head = malloc(sizeof(*list_head));
		if (list_head == NULL) {
			free(frag);
			free(entry);
			abort();
		}

		memset(entry, 0, sizeof(*entry));
		entry->id = id;
		entry->len = 0;
		entry->holes = 1;
		entry->frags = list_head;
		entry->hash = hash;
		entry->protocol = proto;
		entry->timeout.tv_sec = timestamp->tv_sec + reasm->timeout.tv_sec;
		entry->state = STATE_ACTIVE;
		entry->prev = NULL;
		entry->next = reasm->table[hash];
		entry->time_prev = reasm->time_last;
		entry->time_next = NULL;

		memset(list_head, 0, sizeof(*list_head));
		list_head->len = 0;
		list_head->offset = 0;
		list_head->data_offset = 0;
		list_head->data = NULL;

		if (entry->next != NULL)
			entry->next->prev = entry;
		reasm->table[hash] = entry;

		if (reasm->time_last != NULL)
			reasm->time_last->time_next = entry;
		else
			reasm->time_first = entry;
		reasm->time_last = entry;

		reasm->waiting++;
		if (reasm->waiting > reasm->max_waiting)
			reasm->max_waiting = reasm->waiting;
	}

	if (entry->state != STATE_ACTIVE) {
		reasm->dropped_frags++;
		free(frag->data);
		free(frag);
		*out_entry = NULL;
		return (true);
	}

	if (!reasm_add_fragment(entry, frag, last_frag)) {
		entry->state = STATE_INVALID;
		reasm->dropped_frags += entry->frag_count + 1;
		free(frag->data);
		free(frag);
		*out_entry = NULL;
		return (true);
	}

	if (!reasm_is_complete(entry)) {
		*out_entry = NULL;
		return (true);
	}

	remove_entry(reasm, entry);
	*out_entry = entry;
	return (true);
}

bool
reasm_add_fragment(struct reasm_ip_entry *entry, struct reasm_frag_entry *frag, bool last_frag) {
	bool fit_left, fit_right;
	struct reasm_frag_entry *cur, *next;

	/*
	 * When a fragment is inserted into the list, different cases can occur
	 * concerning the number of holes.
	 * - The new fragment can be inserted in the middle of a hole, such that
	 *   it will split the hole in two. The number of holes increases by 1.
	 * - The new fragment can be attached to one end of a hole, such that
	 *   the rest of the hole remains at the opposite side of the fragment.
	 *   The number of holes remains constant.
	 * - The new fragment can fill a hole completely. The number of holes
	 *   decreases by 1.
	 */

	/*
	 * If more fragments follow and the payload size is not an integer
	 * multiple of 8, the packet will never be reassembled completely.
	 */
	if (!last_frag && (frag->len & 7) != 0)
		return (false);

	if (entry->len != 0 && frag->len + frag->offset > entry->len) {
		/* fragment extends past end of packet */
		return (false);
	}

	fit_left = false;
	fit_right = false;

	if (last_frag) {
		if (entry->len != 0)
			return (false);
		entry->len = frag->offset + frag->len;
		fit_right = true;
	}

	cur = entry->frags;
	next = cur->next;

	while (cur->next != NULL && cur->next->offset <= frag->offset)
		cur = cur->next;
	next = cur->next;

	/* Fragment is to be inserted between cur and next; next may be NULL. */

	/* Overlap checks. */
	if (cur->offset + cur->len > frag->offset) {
		/* overlaps with cur */
		return (false);
	} else if (cur->offset + cur->len == frag->offset) {
		fit_left = true;
	}

	if (next != NULL) {
		if (last_frag) {
			/* next extends past end of packet */
			return (false);
		}
		if (frag->offset + frag->len > next->offset) {
			/* overlaps with next */
			return (false);
		}
		else if (frag->offset + frag->len == next->offset)
			fit_right = true;
	}

	/*
	 * Everything's fine, insert it.
	 */
	if (frag->len != 0) {
		frag->next = cur->next;
		cur->next = frag;

		if (fit_left && fit_right)
			entry->holes--;
		else if (!fit_left && !fit_right)
			entry->holes++;

		entry->frag_count++;
	} else {
		/*
		 * If the fragment has zero size, we don't insert it into the list,
		 * but one case remains to be handled: If the zero-size fragment
		 * is the last fragment, and fits exactly with the fragment to its
		 * left, the number of holes decreases.
		 */
		if (last_frag && fit_left)
			entry->holes--;
	}

	return (true);
}

struct reasm_ip *
reasm_ip_new(void) {
	struct reasm_ip *reasm = malloc(sizeof(*reasm));
	if (reasm == NULL)
		return (NULL);

	memset(reasm, 0, sizeof(*reasm));
	return (reasm);
}

void
reasm_ip_free(struct reasm_ip *reasm) {
	while (reasm->time_first != NULL) {
		struct reasm_ip_entry *entry;

		entry = reasm->time_first;
		remove_entry(reasm, entry);
		reasm_free_entry(entry);
	}
	free(reasm);
}

bool
reasm_is_complete(struct reasm_ip_entry *entry) {
	return (entry->holes == 0);
}

void
reasm_assemble(struct reasm_ip_entry *entry, uint8_t *out_packet, size_t *output_len) {
	struct reasm_frag_entry *frag = entry->frags->next; /* skip list head */
	unsigned offset0 = frag->data_offset;

	switch (entry->protocol) {
		case PROTO_IPV4:
			break;
		case PROTO_IPV6:
			offset0 -= 8; /* size of frag header */
			break;
		default:
			abort();
	}

	if (entry->len + offset0 > *output_len) {
		/* The output buffer is too small. */
		*output_len = 0;
		return;
	}

	*output_len = entry->len + offset0;

	/* copy the (unfragmentable) header from the first fragment received */
	memcpy(out_packet, frag->data, offset0);
	if (entry->protocol == PROTO_IPV6)
		out_packet[frag->last_nxt] = frag->ip6f_nxt;

	/* join all the payload fragments together */
	while (frag != NULL) {
		memcpy(out_packet + offset0 + frag->offset,
		       frag->data + frag->data_offset,
		       frag->len);
		frag = frag->next;
	}

	/* some cleanups, e.g. update the length field of reassembled packet */
	switch (entry->protocol) {
		case PROTO_IPV4: {
			struct nmsg_iphdr *ip_header = (struct nmsg_iphdr *) out_packet;
			unsigned i, hl = 4 * ip_header->ip_hl;
			int32_t sum = 0;
			ip_header->ip_len = htons(offset0 + entry->len);
			ip_header->ip_off = 0;
			ip_header->ip_sum = 0;

			/* Recompute checksum. */
			for (i = 0; i < hl; i += 2) {
				uint16_t cur = (uint16_t) out_packet[i] << 8 | out_packet[i + 1];
				sum += cur;
				if ((sum & 0x80000000) != 0)
					sum = (sum & 0xffff) + (sum >> 16);
			}
			while ((sum >> 16) != 0)
				sum = (sum & 0xffff) + (sum >> 16);
			ip_header->ip_sum = htons(~sum);
			break;
		}
		case PROTO_IPV6: {
			uint16_t plen = offset0 + entry->len - 40;
			store_net16(out_packet + offsetof(struct ip6_hdr, ip6_plen), plen);
			break;
		}
		default:
			abort();
	}
}

static void
remove_entry(struct reasm_ip *reasm, struct reasm_ip_entry *entry) {
	if (entry->prev != NULL)
		entry->prev->next = entry->next;
	else
		reasm->table[entry->hash] = entry->next;

	if (entry->next != NULL)
		entry->next->prev = entry->prev;

	if (entry->time_prev != NULL)
		entry->time_prev->time_next = entry->time_next;
	else
		reasm->time_first = entry->time_next;

	if (entry->time_next != NULL)
		entry->time_next->time_prev = entry->time_prev;
	else
		reasm->time_last = entry->time_prev;

	reasm->waiting--;
}

void
reasm_free_entry(struct reasm_ip_entry *entry) {
	struct reasm_frag_entry *frag = entry->frags, *next;
	while (frag != NULL) {
		next = frag->next;
		if (frag->data != NULL)
			free(frag->data);
		free(frag);
		frag = next;
	}
	free(entry);
}

unsigned
reasm_ip_waiting(const struct reasm_ip *reasm) {
	return (reasm->waiting);
}

unsigned
reasm_ip_max_waiting(const struct reasm_ip *reasm) {
	return (reasm->max_waiting);
}

unsigned
reasm_ip_timed_out(const struct reasm_ip *reasm) {
	return (reasm->timed_out);
}

unsigned
reasm_ip_dropped_frags(const struct reasm_ip *reasm) {
	return (reasm->dropped_frags);
}

bool
reasm_ip_set_timeout(struct reasm_ip *reasm, const struct timespec *timeout) {
	if (reasm->time_first != NULL)
		return (false);
	memcpy(&reasm->timeout, timeout, sizeof(*timeout));
	return (true);
}

static void
process_timeouts(struct reasm_ip *reasm, const struct timespec *now) {
	while (reasm->time_first != NULL &&
	       reasm->time_first->timeout.tv_sec < now->tv_sec)
	{
		struct reasm_ip_entry *entry;

		entry = reasm->time_first;
		remove_entry(reasm, entry);
		reasm_free_entry(entry);
		reasm->timed_out++;
	}
}

static struct reasm_frag_entry *
frag_from_ipv6(const uint8_t *packet, uint32_t *ip_id, bool *last_frag) {
	unsigned offset = 40; /* IPv6 header size */
	uint8_t nxt;
	uint16_t total_len;
	unsigned last_nxt = offsetof(struct ip6_hdr, ip6_nxt);
	struct reasm_frag_entry *frag;
	uint8_t *frag_data;

	nxt = packet[offsetof(struct ip6_hdr, ip6_nxt)];
	load_net16(packet + offsetof(struct ip6_hdr, ip6_plen), &total_len);
	total_len += 40;

	/*
	 * IPv6 extension headers from RFC 2460:
	 *   0 Hop-by-Hop Options
	 *  43 Routing
	 *  44 Fragment
	 *  60 Destination Options
	 *
	 * We look out for the Fragment header; the other 3 header
	 * types listed above are recognized and considered safe to
	 * skip over if they occur before the Fragment header.
	 * Any unrecognized header will cause processing to stop and
	 * a subsequent Fragment header to stay unrecognized.
	 */
	while (nxt == IPPROTO_HOPOPTS || nxt == IPPROTO_ROUTING || nxt == IPPROTO_DSTOPTS) {
		unsigned exthdr_len;

		if (offset + 2 > total_len) {
			/* header extends past end of packet */
			return (NULL);
		}

		exthdr_len = 8 + 8 * packet[offset + 1];
		if (offset + exthdr_len > total_len) {
			/* header extends past end of packet */
			return NULL;
		}

		nxt = packet[offset];
		last_nxt = offset;
		offset += exthdr_len;
	}

	if (nxt != IPPROTO_FRAGMENT)
		return (NULL);

	if (offset + 8 > total_len) {
		/* Fragment header extends past end of packet */
		return (NULL);
	}

	frag = malloc(sizeof(*frag));
	if (frag == NULL)
		abort();

	struct ip6_frag frag_hdr;
	memcpy(&frag_hdr, packet + offset, sizeof(struct ip6_frag));
	offset += 8;

	frag_data = malloc(total_len);
	if (frag_data == NULL)
		abort();
	memcpy(frag_data, packet, total_len);

	memset(frag, 0, sizeof(*frag));
	frag->last_nxt = last_nxt;
	frag->ip6f_nxt = frag_hdr.ip6f_nxt;
	frag->len = total_len - offset;
	frag->data_offset = offset;
	frag->offset = ntohs(frag_hdr.ip6f_offlg & IP6F_OFF_MASK);
	frag->data = frag_data;

	*ip_id = ntohl(frag_hdr.ip6f_ident);
	*last_frag = (frag_hdr.ip6f_offlg & IP6F_MORE_FRAG) == 0;

	return (frag);
}

static bool
reasm_id_equal(enum reasm_proto proto, const union reasm_id *left, const union reasm_id *right)
{
	switch (proto) {
		case PROTO_IPV4:
			return (memcmp(left->ipv4.ip_src, right->ipv4.ip_src, 4) == 0 &&
				memcmp(left->ipv4.ip_dst, right->ipv4.ip_dst, 4) == 0 &&
				left->ipv4.ip_id == right->ipv4.ip_id &&
				left->ipv4.ip_proto == right->ipv4.ip_proto);
		case PROTO_IPV6:
			return (memcmp(left->ipv6.ip_src, right->ipv6.ip_src, 16) == 0 &&
				memcmp(left->ipv6.ip_dst, right->ipv6.ip_dst, 16) == 0 &&
				left->ipv6.ip_id == right->ipv6.ip_id);
		default:
			abort();
	}
}

struct reasm_frag_entry *
reasm_parse_packet(const uint8_t *packet, unsigned len,
		   const struct timespec *ts,
		   enum reasm_proto *protocol, union reasm_id *id,
		   unsigned *hash, bool *last_frag)
{
	const struct nmsg_iphdr *ip_header = (const struct nmsg_iphdr *) packet;
	struct reasm_frag_entry *frag = NULL;

	switch (ip_header->ip_v) {
		case 4: {
			uint16_t offset = ntohs(ip_header->ip_off);

			*protocol = PROTO_IPV4;
			if (len >= (unsigned) ntohs(ip_header->ip_len) &&
			    (offset & (IP_MF | IP_OFFMASK)) != 0)
			{
				unsigned pl_hl, pl_len, pl_off;
				u_char *frag_data;

				frag = malloc(sizeof(*frag));
				if (frag == NULL)
					abort();

				pl_hl = ip_header->ip_hl * 4;
				pl_len = ntohs(ip_header->ip_len);
				pl_off = (offset & IP_OFFMASK) * 8;
				frag_data = malloc(pl_len);
				if (frag_data == NULL)
					abort();
				memcpy(frag_data, packet, pl_len);

				frag->len = pl_len - pl_hl;
				frag->offset = pl_off;
				frag->data_offset = ip_header->ip_hl * 4;
				frag->data = frag_data;

				*last_frag = (offset & IP_MF) == 0;

				memcpy(id->ipv4.ip_src, &ip_header->ip_src, 4);
				memcpy(id->ipv4.ip_dst, &ip_header->ip_dst, 4);
				id->ipv4.ip_id = ntohs(ip_header->ip_id);
				id->ipv4.ip_proto = ip_header->ip_p;

				*hash = reasm_ipv4_hash(&id->ipv4);
			}
			break;
		}

		case 6: {
			uint16_t plen;
			load_net16(packet + offsetof(struct ip6_hdr, ip6_plen), &plen);
			*protocol = PROTO_IPV6;
			if (len >= plen + 40U)
				frag = frag_from_ipv6(packet, &id->ipv6.ip_id, last_frag);
			if (frag != NULL) {
				memcpy(id->ipv6.ip_src,
				       packet + offsetof(struct ip6_hdr, ip6_src), 16);
				memcpy(id->ipv6.ip_dst,
				       packet + offsetof(struct ip6_hdr, ip6_dst), 16);
				*hash = reasm_ipv6_hash(&id->ipv6);
			}
			break;
		}

		default:
			break;
	}

	if (frag != NULL)
		memcpy(&frag->ts, ts, sizeof(*ts));

	return (frag);
}
