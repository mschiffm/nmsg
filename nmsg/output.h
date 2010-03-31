/*
 * Copyright (c) 2008, 2009 by Internet Systems Consortium, Inc. ("ISC")
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

#ifndef NMSG_OUTPUT_H
#define NMSG_OUTPUT_H

/*! \file nmsg/output.h
 * \brief Write nmsg containers to output streams.
 *
 * Nmsg payloads can be buffered and written to a file descriptor, or
 * converted to presentation format and written to a file descriptor.
 *
 * <b>MP:</b>
 *	\li Clients must ensure synchronized access when writing to an
 *	nmsg_output_t object.
 *
 * <b>Reliability:</b>
 *	\li Clients must not touch the underlying file descriptor.
 */

#include <sys/types.h>
#include <stdbool.h>

#include <nmsg.h>

/**
 * An enum identifying the underlying implementation of an nmsg_output_t object.
 * This is used for nmsg_io's close event notification.
 */
typedef enum {
	nmsg_output_type_stream,
	nmsg_output_type_pres,
	nmsg_output_type_callback
} nmsg_output_type;

/**
 * Initialize a new byte-stream nmsg output.
 *
 * For efficiency reasons, files should probably be opened with a bufsz of
 * #NMSG_WBUFSZ_MAX.
 *
 * \param[in] fd Writable file descriptor.
 *
 * \param[in] bufsz Value between #NMSG_WBUFSZ_MIN and #NMSG_WBUFSZ_MAX.
 *
 * \return Opaque pointer that is NULL on failure or non-NULL on success.
 */
nmsg_output_t
nmsg_output_open_file(int fd, size_t bufsz);

/**
 * Initialize a new datagram socket nmsg output.
 *
 * For UDP sockets which are physically transported over an Ethernet,
 * #NMSG_WBUFSZ_ETHER or #NMSG_WBUFSZ_JUMBO (for jumbo frame Ethernets) should
 * be used for bufsz.
 *
 * \param[in] fd Writable datagram socket.
 *
 * \param[in] bufsz Value between #NMSG_WBUFSZ_MIN and #NMSG_WBUFSZ_MAX.
 *
 * \return Opaque pointer that is NULL on failure or non-NULL on success.
 */
nmsg_output_t
nmsg_output_open_sock(int fd, size_t bufsz);

/**
 * Initialize a new presentation format (ASCII lines) nmsg output.
 *
 * \param[in] fd Writable file descriptor.
 *
 * \return Opaque pointer that is NULL on failure or non-NULL on success.
 */
nmsg_output_t
nmsg_output_open_pres(int fd);

/**
 * Initialize a new nmsg output closure. This allows a user-provided callback to
 * function as an nmsg output, for instance to participate in an nmsg_io loop.
 * The callback is responsible for disposing of each nmsg message.
 *
 * \param[in] cb Non-NULL function pointer that will be called once for each
 *	payload.
 *
 * \param[in] user Optionally NULL pointer which will be passed to the callback.
 *
 * \return Opaque pointer that is NULL on failure or non-NULL on success.
 */
nmsg_output_t
nmsg_output_open_callback(nmsg_cb_message cb, void *user);

/**
 * Write an nmsg message to an nmsg_output_t object.
 *
 * nmsg_output_write() does not deallocate the nmsg message object. Callers
 * should call nmsg_message_destroy() when finished with a message object.
 *
 * \param[in] output nmsg_output_t object.
 *
 * \param[in] msg nmsg message to be serialized and written to 'output'.
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_failure
 * \return #nmsg_res_nmsg_written
 */
nmsg_res
nmsg_output_write(nmsg_output_t output, nmsg_message_t msg);

/**
 * Close an nmsg_output_t object.
 *
 * \param[in] output Pointer to an nmsg_output_t object.
 *
 * \return #nmsg_res_success
 * \return #nmsg_res_nmsg_written
 */
nmsg_res
nmsg_output_close(nmsg_output_t *output);

/**
 * Make an nmsg_output_t socket output buffered or unbuffered.
 *
 * By default, file and socket nmsg_output_t outputs are buffered. Extremely low
 * volume output streams should probably be unbuffered to reduce latency.
 *
 * \param[in] output Socket nmsg_output_t object.
 *
 * \param[in] buffered True (buffered) or false (unbuffered).
 */
void
nmsg_output_set_buffered(nmsg_output_t output, bool buffered);

/**
 * Filter an nmsg_output_t for a given vendor ID / message type.
 *
 * NMSG messages whose vid and msgtype fields do not match the filter will not
 * be output and will instead be silently discarded.
 *
 * Calling this function with vid=0 and msgtype=0 will disable the filter.
 *
 * \param[in] output nmsg_output_t object.
 *
 * \param[in] vid Vendor ID.
 *
 * \param[in] msgtype Message type.
 */
void
nmsg_output_set_filter_msgtype(nmsg_output_t output, unsigned vid, unsigned msgtype);

/**
 * Filter an nmsg_output_t for a given vendor ID / message type.
 *
 * \param[in] output nmsg_output_t object.
 *
 * \param[in] vname Vendor ID name.
 *
 * \param[in] mname Message type name.
 */
nmsg_res
nmsg_output_set_filter_msgtype_byname(nmsg_output_t output,
				      const char *vname, const char *mname);

/**
 * Limit the payload output rate.
 *
 * \param[in] output nmsg_output_t object.
 *
 * \param[in] rate nmsg_rate_t object or NULL to disable rate limiting.
 */
void
nmsg_output_set_rate(nmsg_output_t output, nmsg_rate_t rate);

/**
 * Set the line continuation string for presentation format output. The default
 * is "\n".
 *
 * \param[in] output nmsg_output_t object.
 *
 * \param[in] endline End-of-line character string.
 */
void
nmsg_output_set_endline(nmsg_output_t output, const char *endline);

/**
 * Set the 'source' field on all output NMSG payloads. This has no effect on
 * non-NMSG outputs.
 *
 * The source ID must be positive.
 *
 * \param[in] output NMSG stream nmsg_output_t object.
 *
 * \param[in] source Source ID.
 */
void
nmsg_output_set_source(nmsg_output_t output, unsigned source);

/**
 * Set the 'operator' field on all output NMSG payloads. This has no effect on
 * non-NMSG outputs.
 *
 * The operator ID must be positive.
 *
 * \param[in] output NMSG stream nmsg_output_t object.
 *
 * \param[in] operator_ Operator ID.
 */
void
nmsg_output_set_operator(nmsg_output_t output, unsigned operator_);

/**
 * Set the 'group' field on all output NMSG payloads. This has no effect on
 * non-NMSG outputs.
 *
 * The group ID must be positive.
 *
 * \param[in] output NMSG stream nmsg_output_t object.
 *
 * \param[in] group Group ID.
 */
void
nmsg_output_set_group(nmsg_output_t output, unsigned group);

/**
 * Enable or disable zlib compression of output NMSG containers.
 *
 * \param[in] output nmsg_output_t object.
 *
 * \param[in] zlibout True (zlib enabled) or false (zlib disabled).
 */
void
nmsg_output_set_zlibout(nmsg_output_t output, bool zlibout);

#endif /* NMSG_OUTPUT_H */
