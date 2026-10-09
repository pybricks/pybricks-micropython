// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

/**
 * @addtogroup Serial pbio/serial: Pybricks Profile over serial byte streams
 *
 * Runs the Pybricks Profile over COBS-framed serial transports (USB, RFCOMM),
 * one connection process per transport.
 * @{
 */

#ifndef _PBIO_SERIAL_H_
#define _PBIO_SERIAL_H_

#include <stdbool.h>

#include <pbio/cobs.h>
#include <pbio/config.h>
#include <pbsys/host.h>

#if PBIO_CONFIG_SERIAL

/**
 * Size of the correlation tag that precedes the host command in a command
 * message. The host picks it and the hub echoes it in the response, see
 * ::PBIO_PYBRICKS_OUT_EP_MSG_COMMAND.
 */
#define PBIO_SERIAL_TAG_SIZE (1)

/**
 * Upper bound on the payload of one serial message, which follows its
 * message type (subscribe, command, read, ...).
 *
 * The largest payload is a command, laid out as:
 *
 *     [tag] [host command]
 *
 * The host command is the same as a BLE command write, so its size already
 * includes its command byte. Events (which include their event byte) and read
 * replies are no larger.
 */
#define PBIO_SERIAL_MAX_PAYLOAD_SIZE (PBIO_SERIAL_TAG_SIZE + PBSYS_CONFIG_HOST_EVENT_OUT_SIZE)

/**
 * Upper bound on one COBS-encoded Pybricks packet, including its trailing
 * frame delimiter.
 *
 * This is the largest thing any serial transport has to carry in one piece.
 * Transports are free to split it, since the stream is reframed on the COBS
 * delimiters, but sizing a transport to this avoids the split.
 */
#define PBIO_SERIAL_MAX_ENCODED_PACKET_SIZE \
    (PBIO_COBS_ENCODED_BUFFER_SIZE(PBIO_SERIAL_MAX_PAYLOAD_SIZE))

/**
 * Initializes the serial connection processes on boot.
 */
void pbio_serial_init(void);

/**
 * De-initializes the serial connection processes on soft-poweroff. USB keeps
 * charging if supported.
 */
void pbio_serial_deinit(void);

/**
 * Indicates if a Pybricks app is connected on the given transport, so the
 * port is open and the host has subscribed to events.
 *
 * @param [in] transport    The transport to test.
 * @retval  true if the connection is active.
 */
bool pbio_serial_connection_is_active(pbsys_host_transport_type_t transport);

/**
 * Notifies the serial process that the host's port state changed. Called by
 * the backing driver, possibly from interrupt context.
 *
 * For USB this is the serial control line state (DTR), meaning a host
 * application opened or closed the serial port. For RFCOMM it is the channel
 * opening or closing.
 *
 * @param [in] transport    The transport whose port state changed.
 * @param [in] open         True if the port is now open, otherwise false.
 */
void pbio_serial_port_changed(pbsys_host_transport_type_t transport, bool open);

#else // PBIO_CONFIG_SERIAL

static inline void pbio_serial_init(void) {
}

static inline void pbio_serial_deinit(void) {
}

static inline bool pbio_serial_connection_is_active(pbsys_host_transport_type_t transport) {
    return false;
}

static inline void pbio_serial_port_changed(pbsys_host_transport_type_t transport, bool open) {
}

#endif // PBIO_CONFIG_SERIAL

#endif // _PBIO_SERIAL_H_

/** @} */
