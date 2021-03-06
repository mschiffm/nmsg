/*
 * Copyright (c) 2009-2012 by Farsight Security, Inc.
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

/* Import. */

#include <pcap.h>

#include "private.h"

/* Macros. */

#define advance_pkt(pkt, len, sz) do { \
	(pkt) += (sz); \
	(len) -= (sz); \
} while (0)

/* Export. */

nmsg_res
nmsg_ipdg_parse(struct nmsg_ipdg *dg, unsigned etype, size_t len,
		const u_char *pkt)
{
	return (_nmsg_ipdg_parse_reasm(dg, etype, len, pkt,
				       NULL, NULL, NULL, NULL, 0));
}

nmsg_res
nmsg_ipdg_parse_pcap(struct nmsg_ipdg *dg, struct nmsg_pcap *pcap,
		     struct pcap_pkthdr *pkt_hdr, const u_char *pkt)
{
	int defrag = 0;
	size_t len = pkt_hdr->caplen;
	unsigned etype = 0;
	unsigned new_len = NMSG_IPSZ_MAX;
	nmsg_res res;

	/* only operate on complete packets */
	if (pkt_hdr->caplen != pkt_hdr->len)
		return (nmsg_res_again);

	/* process data link header */
	switch (pcap->datalink) {
	case DLT_EN10MB: {
		const struct nmsg_ethhdr *eth;

		if (len < sizeof(*eth))
			return (nmsg_res_again);

		eth = (const struct nmsg_ethhdr *) pkt;
		advance_pkt(pkt, len, sizeof(struct nmsg_ethhdr));
		load_net16(&eth->ether_type, &etype);
		if (etype == ETHERTYPE_VLAN) {
			if (len < 4)
				return (nmsg_res_again);
			advance_pkt(pkt, len, 2);
			load_net16(pkt, &etype);
			advance_pkt(pkt, len, 2);
		}
		break;
	}
	case DLT_RAW: {
		const struct nmsg_iphdr *ip;

		if (len < sizeof(*ip))
			return (nmsg_res_again);
		ip = (const struct nmsg_iphdr *) pkt;

		if (ip->ip_v == 4) {
			etype = ETHERTYPE_IP;
		} else if (ip->ip_v == 6) {
			etype = ETHERTYPE_IPV6;
		} else {
			return (nmsg_res_again);
		}
		break;
	}
#ifdef DLT_LINUX_SLL
	case DLT_LINUX_SLL: {
		if (len < 16)
			return (nmsg_res_again);
		advance_pkt(pkt, len, ETHER_HDR_LEN);
		load_net16(pkt, &etype);
		advance_pkt(pkt, len, 2);
		break;
	}
#endif
	} /* end switch */

	res = _nmsg_ipdg_parse_reasm(dg, etype, len, pkt, pcap->reasm,
				     &new_len, pcap->new_pkt, &defrag,
				     pkt_hdr->ts.tv_sec);
	if (res == nmsg_res_success && defrag == 1) {
		/* refilter the newly reassembled datagram */
		struct bpf_insn *fcode = pcap->userbpf.bf_insns;

		if (fcode != NULL &&
		    bpf_filter(fcode, (u_char *) dg->network, dg->len_network,
			       dg->len_network) == 0)
		{
			return (nmsg_res_again);
		}
	}
	return (res);
}

nmsg_res
_nmsg_ipdg_parse_reasm(struct nmsg_ipdg *dg, unsigned etype, size_t len,
		       const u_char *pkt, struct _nmsg_ipreasm *reasm,
		       unsigned *new_len, u_char *new_pkt, int *defrag,
		       uint64_t timestamp)
{
	bool is_fragment = false;
	unsigned frag_hdr_offset = 0;
	unsigned tp_payload_len = 0;

	dg->network = pkt;
	dg->len_network = len;

	/* process network header */
	switch (etype) {
	case ETHERTYPE_IP: {
		const struct nmsg_iphdr *ip;
		unsigned ip_off;

		ip = (const struct nmsg_iphdr *) dg->network;
		advance_pkt(pkt, len, ip->ip_hl << 2);

		load_net16(&ip->ip_off, &ip_off);
		if ((ip_off & IP_OFFMASK) != 0 ||
		    (ip_off & IP_MF) != 0)
		{
			is_fragment = true;
		}
		dg->proto_network = PF_INET;
		dg->proto_transport = ip->ip_p;
		break;
	}
	case ETHERTYPE_IPV6: {
		const struct ip6_hdr *ip6;
		uint16_t payload_len;
		uint8_t nexthdr;
		unsigned thusfar;

		if (len < sizeof(*ip6))
			return (nmsg_res_again);
		ip6 = (const struct ip6_hdr *) dg->network;
		if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION)
			return (nmsg_res_again);

		nexthdr = ip6->ip6_nxt;
		thusfar = sizeof(struct ip6_hdr);
		load_net16(&ip6->ip6_plen, &payload_len);

		while (nexthdr == IPPROTO_ROUTING ||
		       nexthdr == IPPROTO_HOPOPTS ||
		       nexthdr == IPPROTO_FRAGMENT ||
		       nexthdr == IPPROTO_DSTOPTS ||
		       nexthdr == IPPROTO_AH ||
		       nexthdr == IPPROTO_ESP)
		{
			struct {
				uint8_t nexthdr;
				uint8_t length;
			} ext_hdr;
			uint16_t ext_hdr_len;

			/* catch broken packets */
			if ((thusfar + sizeof(ext_hdr)) > len)
			    return (nmsg_res_again);

			if (nexthdr == IPPROTO_FRAGMENT) {
				frag_hdr_offset = thusfar;
				is_fragment = true;
				break;
			}

			memcpy(&ext_hdr, (const u_char *) ip6 + thusfar,
			       sizeof(ext_hdr));
			nexthdr = ext_hdr.nexthdr;
			ext_hdr_len = (8 * (ntohs(ext_hdr.length) + 1));

			if (ext_hdr_len > payload_len)
				return (nmsg_res_again);

			thusfar += ext_hdr_len;
			payload_len -= ext_hdr_len;
		}

		if ((thusfar + payload_len) > len || payload_len == 0)
			return (nmsg_res_again);

		advance_pkt(pkt, len, thusfar);

		dg->proto_network = PF_INET6;
		dg->proto_transport = nexthdr;
		break;
	}
	default:
		return (nmsg_res_again);
		break;
	} /* end switch */

	/* handle IPv4 and IPv6 fragments */
	if (is_fragment == true && reasm != NULL) {
		bool rres;

		rres = reasm_ip_next(reasm, dg->network, dg->len_network,
				     frag_hdr_offset, timestamp,
				     new_pkt, new_len);
		if (rres == false || *new_len == 0) {
			/* not all fragments have been received */
			return (nmsg_res_again);
		}
		/* the datagram has been fully reassembled */
		if (defrag != NULL)
			*defrag = 1;
		return (nmsg_ipdg_parse(dg, etype, *new_len, new_pkt));
	}
	if (is_fragment == true && reasm == NULL)
		return (nmsg_res_again);

	dg->transport = pkt;
	dg->len_transport = len;

	/* process transport header */
	switch (dg->proto_transport) {
	case IPPROTO_UDP: {
		struct nmsg_udphdr *udp;

		if (len < sizeof(struct nmsg_udphdr))
			return (nmsg_res_again);
		udp = (struct nmsg_udphdr *) pkt;
		load_net16(&udp->uh_ulen, &tp_payload_len);
		if (tp_payload_len >= 8)
			tp_payload_len -= 8;
		advance_pkt(pkt, len, sizeof(struct nmsg_udphdr));

		dg->payload = pkt;
		dg->len_payload = tp_payload_len;
		if (dg->len_payload > len)
			dg->len_payload = len;
		break;
	}
	default:
		return (nmsg_res_again);
		break;
	} /* end switch */

	return (nmsg_res_success);
}

nmsg_res
nmsg_ipdg_parse_pcap_raw(struct nmsg_ipdg *dg, int datalink, const uint8_t *pkt, size_t len)
{
	bool is_initial_fragment = false;
	bool is_fragment = false;
	unsigned etype = 0;
	unsigned tp_payload_len = 0;

	/* process data link header */
	switch (datalink) {
	case DLT_EN10MB: {
		const struct nmsg_ethhdr *eth;

		if (len < sizeof(*eth))
			return (nmsg_res_again);

		eth = (const struct nmsg_ethhdr *) pkt;
		advance_pkt(pkt, len, sizeof(struct nmsg_ethhdr));
		load_net16(&eth->ether_type, &etype);
		if (etype == ETHERTYPE_VLAN) {
			if (len < 4)
				return (nmsg_res_again);
			advance_pkt(pkt, len, 2);
			load_net16(pkt, &etype);
			advance_pkt(pkt, len, 2);
		}
		break;
	}
	case DLT_RAW: {
		const struct nmsg_iphdr *ip;

		if (len < sizeof(*ip))
			return (nmsg_res_again);
		ip = (const struct nmsg_iphdr *) pkt;

		if (ip->ip_v == 4) {
			etype = ETHERTYPE_IP;
		} else if (ip->ip_v == 6) {
			etype = ETHERTYPE_IPV6;
		} else {
			return (nmsg_res_again);
		}
		break;
	}
#ifdef DLT_LINUX_SLL
	case DLT_LINUX_SLL: {
		if (len < 16)
			return (nmsg_res_again);
		advance_pkt(pkt, len, ETHER_HDR_LEN);
		load_net16(pkt, &etype);
		advance_pkt(pkt, len, 2);
		break;
	}
#endif
	} /* end switch */

	dg->network = pkt;
	dg->len_network = len;

	/* process network header */
	switch (etype) {
	case ETHERTYPE_IP: {
		const struct nmsg_iphdr *ip;
		unsigned ip_off;

		ip = (const struct nmsg_iphdr *) dg->network;
		advance_pkt(pkt, len, ip->ip_hl << 2);

		load_net16(&ip->ip_off, &ip_off);
		if ((ip_off & IP_OFFMASK) != 0 ||
		    (ip_off & IP_MF) != 0)
		{
			is_fragment = true;
		}
		if ((ip_off & IP_OFFMASK) == 0)
			is_initial_fragment = true;
		dg->proto_network = PF_INET;
		dg->proto_transport = ip->ip_p;
		break;
	}
	case ETHERTYPE_IPV6: {
		const struct ip6_hdr *ip6;
		const struct ip6_frag *frag;
		uint16_t payload_len;
		uint8_t nexthdr;
		unsigned thusfar;

		if (len < sizeof(*ip6))
			return (nmsg_res_again);
		ip6 = (const struct ip6_hdr *) dg->network;
		if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION)
			return (nmsg_res_again);

		nexthdr = ip6->ip6_nxt;
		thusfar = sizeof(struct ip6_hdr);
		load_net16(&ip6->ip6_plen, &payload_len);

		while (nexthdr == IPPROTO_ROUTING ||
		       nexthdr == IPPROTO_HOPOPTS ||
		       nexthdr == IPPROTO_FRAGMENT ||
		       nexthdr == IPPROTO_DSTOPTS ||
		       nexthdr == IPPROTO_AH ||
		       nexthdr == IPPROTO_ESP)
		{
			struct {
				uint8_t nexthdr;
				uint8_t length;
			} ext_hdr;
			uint16_t ext_hdr_len;

			/* catch broken packets */
			if ((thusfar + sizeof(ext_hdr)) > len)
			    return (nmsg_res_again);

			if (nexthdr == IPPROTO_FRAGMENT) {
				is_fragment = true;
				frag = (const struct ip6_frag *) (dg->network + thusfar);
				if ((frag->ip6f_offlg & IP6F_OFF_MASK) == 0)
					is_initial_fragment = true;
			}

			memcpy(&ext_hdr, (const u_char *) ip6 + thusfar,
			       sizeof(ext_hdr));
			nexthdr = ext_hdr.nexthdr;
			ext_hdr_len = (8 * (ntohs(ext_hdr.length) + 1));

			if (ext_hdr_len > payload_len)
				return (nmsg_res_again);

			thusfar += ext_hdr_len;
			payload_len -= ext_hdr_len;

			if (is_fragment)
				break;
		}

		if ((thusfar + payload_len) > len || payload_len == 0)
			return (nmsg_res_again);

		advance_pkt(pkt, len, thusfar);

		dg->proto_network = PF_INET6;
		dg->proto_transport = nexthdr;
		break;
	}
	default:
		return (nmsg_res_again);
		break;
	} /* end switch */

	if (is_fragment) {
		if (is_initial_fragment) {
			dg->transport = pkt;
			dg->len_transport = len;
		} else {
			dg->transport = NULL;
			dg->len_transport = 0;
			dg->payload = pkt;
			dg->len_payload = len;
			return (nmsg_res_success);
		}
	} else {
		dg->transport = pkt;
		dg->len_transport = len;
	}

	/* process transport header */
	switch (dg->proto_transport) {
	case IPPROTO_UDP: {
		struct nmsg_udphdr *udp;

		if (len < sizeof(struct nmsg_udphdr))
			return (nmsg_res_again);
		udp = (struct nmsg_udphdr *) pkt;
		load_net16(&udp->uh_ulen, &tp_payload_len);
		if (tp_payload_len >= 8)
			tp_payload_len -= 8;
		advance_pkt(pkt, len, sizeof(struct nmsg_udphdr));

		dg->payload = pkt;
		dg->len_payload = tp_payload_len;
		if (dg->len_payload > len)
			dg->len_payload = len;
		break;
	}
	case IPPROTO_TCP: {
		struct nmsg_tcphdr *tcp;

		if (len < sizeof(struct nmsg_tcphdr))
			return (nmsg_res_again);
		tcp = (struct nmsg_tcphdr *) pkt;
		tp_payload_len = dg->len_network - 4 * tcp->th_off;
		advance_pkt(pkt, len, 4 * tcp->th_off);
		dg->payload = pkt;
		dg->len_payload = tp_payload_len;
		if (dg->len_payload > len)
			dg->len_payload = len;
		break;
	}
	case IPPROTO_ICMP: {
		if (len < sizeof(struct nmsg_icmphdr))
			return (nmsg_res_again);
		advance_pkt(pkt, len, sizeof(struct nmsg_icmphdr));
		dg->payload = pkt;
		dg->len_payload = len;
		break;
	}
	default:
		return (nmsg_res_again);
		break;
	} /* end switch */

	return (nmsg_res_success);
}
