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

#ifndef NMSG_PCAP_H
#define NMSG_PCAP_H

/*! \file nmsg/pcap_input.h
 * \brief Reassembled IP datagram interface to libpcap.
 *
 * libpcap's frame-based interface is wrapped with calls to the ipdg.h interface
 * and provides the caller with reassembled IP datagrams.
 *
 * Callers should not call pcap_setfilter() on the pcap_t handle passed to
 * nmsg_pcap_input_open() but should instead use nmsg_pcap_input_setfilter().
 * Since IP datagrams are reassembled in userspace, they must undergo
 * reevaluation of the user-provided filter. nmsg_pcap_input_setfilter() and
 * nmsg_pcap_input_read() handle this transparently.
 */

#include <nmsg.h>
#include <pcap.h>

/**
 * Initialize a new nmsg_pcap_t input from a libpcap source.
 *
 * \param[in] phandle pcap_t handle (e.g., acquired from pcap_open_offline() or
 * pcap_open_live()).
 *
 * \return Opaque pointer that is NULL on failure or non-NULL on success.
 */
nmsg_pcap_t
nmsg_pcap_input_open(pcap_t *phandle);

/**
 * Close an nmsg_pcap_t object and release all associated resources.
 *
 * \param[in] pcap pointer to an nmsg_pcap_t object.
 */
nmsg_res
nmsg_pcap_input_close(nmsg_pcap_t *pcap);

/**
 * Read an IP datagram from an nmsg_pcap_t input, performing reassembly if
 * necessary.
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \param[out] dg nmsg_ipdg structure to be filled.
 *
 * \param[out] ts timespec structure indicating time of datagram reception.
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_again
 */
nmsg_res
nmsg_pcap_input_read(nmsg_pcap_t pcap, struct nmsg_ipdg *dg,
		     struct timespec *ts);

/**
 * Set a bpf filter on an nmsg_pcap_t object.
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \param[in] bpfstr is a valid bpf filter expression that will be passed to
 *	pcap_compile().
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_failure
 */
nmsg_res
nmsg_pcap_input_setfilter(nmsg_pcap_t pcap, const char *bpfstr);

#endif /* NMSG_PCAP_H */
