/*
 * Copyright (c) 2009 by Internet Systems Consortium, Inc. ("ISC")
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

#ifndef NMSG_IPDG_H
#define NMSG_IPDG_H

/*! \file nmsg/ipdg.h
 * \brief IP datagram parsing functions.
 *
 * These functions parse network packets or ethernet frames and return
 * IP datagrams, performing reassembly if requested.  Non-IP packets are
 * discarded.
 */

#include <sys/types.h>
#include <stdint.h>

#include <pcap.h>
#include <nmsg.h>

/**
 * Parsed IP datagram.
 */
struct nmsg_ipdg {
	int		proto_network;	 /*%< PF_* value */
	int		proto_transport; /*%< transport protocol */
	unsigned	len_network;	 /*%< length starting from network */
	unsigned	len_transport;	 /*%< length starting from transport */
	unsigned	len_payload;	 /*%< length starting from payload */
	const u_char	*network;	 /*%< pointer to network header */
	const u_char	*transport;	 /*%< pointer to transport header */
	const u_char	*payload;	 /*%< pointer to application payload */
};

/**
 * Parse IP packets from the network layer, discarding fragments.
 *
 * Populate a struct nmsg_ipdg indicating where the network, transport, and
 * payload sections of the packet are and the length of the remaining packet at
 * each of those sections.
 *
 * This function operates on datagrams from the network layer.
 *
 * Broken and fragmented datagrams are discarded.
 *
 * \param[out] dg caller-allocated struct nmsg_ipdg which will be populated
 *	after a successful call.
 *
 * \param[in] etype ETHERTYPE_* value. The only supported values are
 *	ETHERTYPE_IP and ETHERTYPE_IPV6.
 *
 * \param[in] len length of the packet.
 *
 * \param[in] pkt pointer to the packet.
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_again
 */
nmsg_res
nmsg_ipdg_parse(struct nmsg_ipdg *dg, unsigned etype, size_t len,
		const u_char *pkt);

/**
 * Parse IP datagrams from the data link layer, performing reassembly if
 * necessary.
 *
 * Populate a struct nmsg_ipdg indicating where the network, transport, and
 * payload sections of the datagram are and the length of the remaining packet
 * at each of those sections.
 *
 * This function operates on raw frames returned by libpcap from the data
 * link layer. The packet beginning at 'pkt' must match the datalink type
 * associated with 'pcap' and must be pkt_hdr->caplen octets long.
 *
 * libpcap data link types DLT_EN10MB, DLT_RAW, and DLT_LINUX_SLL are supported.
 *
 * Broken packets are discarded. All but the final fragment of a fragmented
 * datagram are stored internally and #nmsg_res_again is returned.
 *
 * \param[out] dg caller-allocated struct nmsg_ipdg which will be populated
 *	after a successful call.
 *
 * \param[in] pcap caller-initialized nmsg_pcap object from whose pcap handle
 *	the packet 'pkt' was received.
 *
 * \param[in] pkt_hdr pointer to the pcap packet header corresponding to 'pkt'.
 *
 * \param[in] pkt pointer to the packet.
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_again
 */
nmsg_res
nmsg_ipdg_parse_pcap(struct nmsg_ipdg *dg, nmsg_pcap_t pcap,
		     struct pcap_pkthdr *pkt_hdr, const u_char *pkt);


#endif /* NMSG_IPDG_H */
