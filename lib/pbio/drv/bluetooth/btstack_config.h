// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// BTstack build configuration, derived from the pbdrv Bluetooth config.
//
// BTstack includes "btstack_config.h" by that fixed name from its own sources,
// so this file has to be called that and be on the include path. There is one
// of it for every platform: everything below follows from the
// PBDRV_CONFIG_BLUETOOTH_* flags the platform already declares, so the
// features a platform enables and the static resources that pay for them
// cannot drift apart.
//
// bluetooth_btstack.c is the reference for every count below: each service,
// channel and record it registers is accounted for here, under the same flag
// that makes the driver register it.

#ifndef _PBDRV_BLUETOOTH_BTSTACK_CONFIG_H_
#define _PBDRV_BLUETOOTH_BTSTACK_CONFIG_H_

#include <pbdrv/bluetooth.h>
#include <pbio/cobs.h>
#include <pbsys/config.h>

// ---------------------------------------------------------------------------
// What the platform runs
//
// The driver tests these flags with #if, where an undefined flag is simply
// zero. The counts below need them as values in ordinary expressions too, so
// they are restated here as plain 0 or 1.
// ---------------------------------------------------------------------------

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HID
#define PBDRV_BTSTACK_HID (1)
#else
#define PBDRV_BTSTACK_HID (0)
#endif

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC_HOST
#define PBDRV_BTSTACK_HOST (1)
#else
#define PBDRV_BTSTACK_HOST (0)
#endif

#if PBDRV_CONFIG_BLUETOOTH_PEER
#define PBDRV_BTSTACK_PEER (1)
#define PBDRV_BTSTACK_PEERS (PBDRV_CONFIG_BLUETOOTH_PEER_MAX_PEERS)
#else
#define PBDRV_BTSTACK_PEER (0)
#define PBDRV_BTSTACK_PEERS (0)
#endif

/** Whether an SDP server runs, which the Classic profiles need. */
#define PBDRV_BTSTACK_SDP (PBDRV_BTSTACK_HID || PBDRV_BTSTACK_HOST)

// ---------------------------------------------------------------------------
// Features
// ---------------------------------------------------------------------------

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_POSIX
// Running on a PC rather than a hub, so BTstack may use the C library for the
// things it would otherwise need a port for.
#define HAVE_ASSERT
#define HAVE_BTSTACK_STDIN
#define HAVE_POSIX_FILE_IO
#define HAVE_POSIX_TIME
#define ENABLE_LOG_ERROR
#define ENABLE_LOG_INFO
#endif

#define ENABLE_BLE
#define ENABLE_LE_CENTRAL
#define ENABLE_LE_PERIPHERAL
#define ENABLE_PRINTF_HEXDUMP

// Classic is built for the peer links as much as for the Classic profiles: a
// peer channel is plain Classic L2CAP, with no profile behind it.
#if PBDRV_BTSTACK_HID || PBDRV_BTSTACK_HOST || PBDRV_BTSTACK_PEER
#define ENABLE_CLASSIC
#endif

// ---------------------------------------------------------------------------
// Link budgets
//
// BTstack sizes two buffers per HCI connection from HCI_ACL_PAYLOAD_SIZE (the
// ACL recombination buffer and the per-connection ATT server request buffer),
// so it sets the static RAM cost of every extra connection. It also silently
// caps every channel MTU at it rather than refusing to register a service, so
// a value that is too small shows up only as packets that are a few bytes too
// big to send in one piece.
//
// Each budget below is what one packet of that link needs inside an ACL
// payload. bluetooth_btstack.c static-asserts each one against the result.
// ---------------------------------------------------------------------------

/** L2CAP header on every ACL payload. BTstack calls this L2CAP_HEADER_SIZE. */
#define PBDRV_BTSTACK_L2CAP_HEADER (4)

/** RFCOMM frame overhead: address, control, 2-byte length field and FCS. */
#define PBDRV_BTSTACK_RFCOMM_HEADER (5)

/** One full ATT PDU, which is the largest thing a BLE link carries. */
#define PBDRV_BTSTACK_BLE_BUDGET \
    (PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE + PBDRV_BTSTACK_L2CAP_HEADER)

/** One whole peer message. Basic mode keeps it in one L2CAP packet. */
#define PBDRV_BTSTACK_PEER_BUDGET \
    (PBDRV_BLUETOOTH_PEER_MTU + PBDRV_BTSTACK_L2CAP_HEADER)

/**
 * One whole COBS-encoded Pybricks packet in a single RFCOMM frame. RFCOMM is
 * a byte stream, so a smaller value still works, but it splits every large
 * packet over two frames for no good reason.
 */
#define PBDRV_BTSTACK_RFCOMM_BUDGET \
    (PBIO_COBS_ENCODED_BUFFER_SIZE(PBSYS_CONFIG_HOST_EVENT_OUT_SIZE) + \
    PBDRV_BTSTACK_RFCOMM_HEADER + PBDRV_BTSTACK_L2CAP_HEADER)

#if PBDRV_BTSTACK_HOST
#define HCI_ACL_PAYLOAD_SIZE PBDRV_BTSTACK_RFCOMM_BUDGET
#elif PBDRV_BTSTACK_PEER
#define HCI_ACL_PAYLOAD_SIZE PBDRV_BTSTACK_PEER_BUDGET
#else
#define HCI_ACL_PAYLOAD_SIZE PBDRV_BTSTACK_BLE_BUDGET
#endif

// ---------------------------------------------------------------------------
// Static resources
// ---------------------------------------------------------------------------

/**
 * One per link that can be up at the same time: the app hosts and the
 * peripherals on BLE, a host computer and a HID device on Classic, and one
 * per peer brick.
 */
#define MAX_NR_HCI_CONNECTIONS ( \
    PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS + \
    PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS + \
    PBDRV_BTSTACK_HOST + PBDRV_BTSTACK_HID + PBDRV_BTSTACK_PEERS)

/** One per peripheral the hub connects to and discovers services on. */
#define MAX_NR_GATT_CLIENTS (PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS)

/**
 * The peer PSM, the SDP server, the two HID channels (control and interrupt)
 * and the RFCOMM PSM. These are the l2cap_register_service() calls the driver
 * makes, directly or through hid_host_init() and rfcomm_init().
 */
#define MAX_NR_L2CAP_SERVICES ( \
    PBDRV_BTSTACK_PEER + PBDRV_BTSTACK_SDP + \
    PBDRV_BTSTACK_HID * 2 + PBDRV_BTSTACK_HOST)

/**
 * One channel per peer brick, the two HID channels, the RFCOMM multiplexer,
 * and an SDP channel each way: the hub queries a HID device it is connecting
 * to, and a device that pages the hub queries it back.
 */
#define MAX_NR_L2CAP_CHANNELS ( \
    PBDRV_BTSTACK_PEERS + PBDRV_BTSTACK_HID * 2 + \
    PBDRV_BTSTACK_HOST + PBDRV_BTSTACK_SDP * 2)

/** One multiplexer, since only one host computer is served at a time. */
#define MAX_NR_RFCOMM_MULTIPLEXERS (PBDRV_BTSTACK_HOST)

/** The one Pybricks serial port. */
#define MAX_NR_RFCOMM_SERVICES (PBDRV_BTSTACK_HOST)

/**
 * The one channel being served, plus one for a second host that connects
 * while it is up and gets declined, which BTstack has already allocated by
 * the time the driver sees it.
 */
#define MAX_NR_RFCOMM_CHANNELS (PBDRV_BTSTACK_HOST * 2)

/** The one gamepad or other HID device. */
#define MAX_NR_HID_HOST_CONNECTIONS (PBDRV_BTSTACK_HID)

/** The Device ID record, and the serial port record when there is one. */
#define MAX_NR_SERVICE_RECORD_ITEMS (PBDRV_BTSTACK_SDP + PBDRV_BTSTACK_HOST)

/** The ATT database the hub serves to a connected app. */
#define MAX_ATT_DB_SIZE 512

/** Bonds are kept in pbio storage, not in a BTstack memory pool. */
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 0

/** No hands-free profile on any platform. */
#define MAX_NR_HFP_CONNECTIONS 0

/** Scanning filters by advertisement content, so no controller whitelist. */
#define MAX_NR_WHITELIST_ENTRIES 0

/** Pairings in progress, and the LE bonds remembered across them. */
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_LE_DEVICE_DB_ENTRIES 3

#endif // _PBDRV_BLUETOOTH_BTSTACK_CONFIG_H_
