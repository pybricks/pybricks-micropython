// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

// BlueKitchen BTStack config

#ifndef _PLATFORM_PRIME_HUB_BTSTACK_CONFIG_H_
#define _PLATFORM_PRIME_HUB_BTSTACK_CONFIG_H_

// BTstack features that can be enabled
#define ENABLE_BLE
// Only for the peer-to-peer L2CAP channels; no Classic profiles are built in.
#define ENABLE_CLASSIC
// #define ENABLE_CC256X_BAUDRATE_CHANGE_FLOWCONTROL_BUG_WORKAROUND
#define ENABLE_LE_CENTRAL
#define ENABLE_LE_PERIPHERAL
#define ENABLE_PRINTF_HEXDUMP

// #define ENABLE_LOG_DEBUG
// #define ENABLE_LOG_ERROR
// #define ENABLE_LOG_INFO

// BTstack configuration. buffers, sizes, ...
// Sized by the largest link, which here is a peer channel: one whole peer
// message (PBDRV_BLUETOOTH_PEER_MTU, 520) plus the 4-byte L2CAP header.
// BTstack would otherwise silently cap the channel MTU rather than refuse
// it. BLE needs slightly less: one full ATT PDU at
// PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE (515) plus the same 4-byte L2CAP
// header. The LE MTU no longer follows this number, since
// pbdrv_bluetooth_init() pins it to the platform maximum.
//
// This is allocated twice per HCI connection (ACL recombination buffer plus
// the per-connection ATT server request buffer), so it dominates the static
// RAM cost of every extra connection. BTstack's Classic default
// of 1691 would pay for an MTU nothing asks for.
#define HCI_ACL_PAYLOAD_SIZE (520 + 4)
#define MAX_ATT_DB_SIZE 512
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES  0
#define MAX_NR_GATT_CLIENTS 2
// BLE host and peripherals as before, plus one per peer hub.
#define MAX_NR_HCI_CONNECTIONS 6 // CC2564C can have up to 10 connections
#define MAX_NR_HFP_CONNECTIONS 0
// One peer channel per peer hub.
#define MAX_NR_L2CAP_CHANNELS  3
// The peer PSM.
#define MAX_NR_L2CAP_SERVICES  1
// No Classic profiles, so no RFCOMM and no SDP records.
#define MAX_NR_RFCOMM_CHANNELS 0
#define MAX_NR_RFCOMM_MULTIPLEXERS 0
#define MAX_NR_RFCOMM_SERVICES 0
#define MAX_NR_SERVICE_RECORD_ITEMS 0
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_WHITELIST_ENTRIES 0
#define MAX_NR_LE_DEVICE_DB_ENTRIES 3

// Link Key DB and LE Device DB using TLV on top of Flash Sector interface
// #define NVM_NUM_DEVICE_DB_ENTRIES 16
// #define NVM_NUM_LINK_KEYS 16

#endif // _PLATFORM_PRIME_HUB_BTSTACK_CONFIG_H_
