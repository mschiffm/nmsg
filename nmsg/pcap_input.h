/*
 * Copyright (c) 2009, 2011 by Farsight Security, Inc.
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

typedef enum {
	nmsg_pcap_type_file,
	nmsg_pcap_type_live
} nmsg_pcap_type;

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
 * \return #nmsg_res_pcap_error
 * \return #nmsg_res_again
 */
nmsg_res
nmsg_pcap_input_read(nmsg_pcap_t pcap, struct nmsg_ipdg *dg,
		     struct timespec *ts);

/**
 * Read a raw packet from an nmsg_pcap_t input.
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \param[out] pkt_hdr Location to store pcap packet header.
 *
 * \param[out] pkt_data Location to store pcap packet data.
 *
 * \param[out] ts timespec structure indicating time of packet reception.
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_pcap_error
 * \return #nmsg_res_again
 */
nmsg_res
nmsg_pcap_input_read_raw(nmsg_pcap_t pcap, struct pcap_pkthdr **pkt_hdr,
			 const uint8_t **pkt_data, struct timespec *ts);

/**
 * Set a bpf filter on an nmsg_pcap_t object. This function performs
 * additional transformation on the bpf filter text before passing the
 * filter to pcap_compile() and pcap_setfilter().
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

/**
 * Set a bpf filter on an nmsg_pcap_t object. This function is a thin
 * wrapper for pcap_setfilter().
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \param[in] bpfstr is a valid bpf filter expression that will be passed to
 *	pcap_compile() and pcap_setfilter().
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_failure
 */
nmsg_res
nmsg_pcap_input_setfilter_raw(nmsg_pcap_t pcap, const char *bpfstr);

/**
 * Set raw mode.
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \param[in] raw True if raw packets should be passed, false if reassembled
 *	datagrams should be passed.
 */
void
nmsg_pcap_input_set_raw(nmsg_pcap_t pcap, bool raw);

/**
 * Get the snapshot length of the underlying pcap handle.
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \return Pcap snapshot length.
 */
int
nmsg_pcap_snapshot(nmsg_pcap_t pcap);

/**
 * Get the type of the underlying pcap handle.
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \return #nmsg_pcap_type_file
 * \return #nmsg_pcap_type_live
 */
nmsg_pcap_type
nmsg_pcap_get_type(nmsg_pcap_t pcap);

/**
 * Get the datalink type of the underlying pcap handle.
 *
 * \param[in] pcap nmsg_pcap_t object.
 */
int
nmsg_pcap_get_datalink(nmsg_pcap_t pcap);

/**
 * Return the result of filtering a packet.
 *
 * \param[in] pcap nmsg_pcap_t object.
 *
 * \param[in] pkt Pointer to start of network packet.
 *
 * \param[in] len Length of packet.
 *
 * \return false if packet failed the filter
 * \return true if packet passed the filter
 */
bool
nmsg_pcap_filter(nmsg_pcap_t pcap, const uint8_t *pkt, size_t len);

#endif /* NMSG_PCAP_H */
