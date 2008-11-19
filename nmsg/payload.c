/*
 * Copyright (c) 2008 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Import. */

#include "nmsg_port.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nmsg.h"
#include "payload.h"

/* Export. */

Nmsg__NmsgPayload *
nmsg_payload_dup(const Nmsg__NmsgPayload *np, ProtobufCAllocator *ca) {
	Nmsg__NmsgPayload *dup;

	dup = ca->alloc(ca->allocator_data, sizeof(*dup));
	if (dup == NULL)
		return (NULL);
	memcpy(dup, np, sizeof(*dup));
	if (np->has_payload) {
		dup->payload.data = ca->alloc(ca->allocator_data,
					      dup->payload.len);
		if (dup->payload.data == NULL) {
			free(dup);
			return (NULL);
		}
		memcpy(dup->payload.data, np->payload.data, np->payload.len);
	}
	return (dup);
}

void
nmsg_payload_free(Nmsg__NmsgPayload **np, ProtobufCAllocator *ca) {
	if (ca->free != NULL && (*np)->has_payload
	    && (*np)->payload.data != NULL)
		ca->free(ca->allocator_data, (*np)->payload.data);
	if (ca->free != NULL)
		ca->free(ca->allocator_data, *np);
	*np = NULL;
}

size_t
nmsg_payload_size(const Nmsg__NmsgPayload *np) {
	size_t sz;
	Nmsg__NmsgPayload copy;

	copy = *np;
	copy.payload.len = 0;
	sz = nmsg__nmsg_payload__get_packed_size(&copy);

	sz += np->payload.len;

	/* varint encoded length */
	if (np->payload.len >= (1 << 7))
		sz += 1;
	if (np->payload.len >= (1 << 14))
		sz += 1;

	return (sz);
}

Nmsg__NmsgPayload *
nmsg_payload_make(uint8_t *pbuf, size_t sz, unsigned vid, unsigned msgtype,
		  const struct timespec *ts, ProtobufCAllocator *ca)
{
	Nmsg__NmsgPayload *np;

	np = ca->alloc(ca->allocator_data, sizeof(*np));
	if (np == NULL)
		return (NULL);
	memset(np, 0, sizeof(*np));
	np->payload.data = ca->alloc(ca->allocator_data, sz);
	if (np->payload.data == NULL) {
		ca->free(ca->allocator_data, np);
		return (NULL);
	}
	np->base.descriptor = &nmsg__nmsg_payload__descriptor;
	np->base.n_unknown_fields = 0;
	np->base.unknown_fields = NULL;
	np->vid = vid;
	np->msgtype = msgtype;
	np->time_sec = ts->tv_sec;
	np->time_nsec = ts->tv_nsec;
	np->has_payload = true;
	memcpy(np->payload.data, pbuf, sz);
	np->payload.len = sz;

	return (np);
}
