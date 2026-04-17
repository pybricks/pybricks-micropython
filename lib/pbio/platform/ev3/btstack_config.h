// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 The Pybricks Authors

// BlueKitchen BTStack config

#ifndef _PLATFORM_EV3_BTSTACK_CONFIG_H_
#define _PLATFORM_EV3_BTSTACK_CONFIG_H_

// BTstack features that can be enabled
#define ENABLE_BLE
#define ENABLE_CLASSIC
// #define ENABLE_CC256X_BAUDRATE_CHANGE_FLOWCONTROL_BUG_WORKAROUND
#define ENABLE_LE_CENTRAL
#define ENABLE_LE_PERIPHERAL
#define ENABLE_PRINTF_HEXDUMP

// Temporary, until I'm sure it's working.

// There are certain debug logs in the rfcomm source files that use the wrong format specifiers at the debug level.
// #define ENABLE_LOG_DEBUG
#define ENABLE_LOG_ERROR
#define ENABLE_LOG_INFO

// BTstack configuration. buffers, sizes, ...
#define HCI_ACL_PAYLOAD_SIZE (1691 + 4)
#define MAX_ATT_DB_SIZE 512
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 20
#define MAX_NR_HCI_CONNECTIONS 7  // CC2560/A limit.
#define MAX_NR_HFP_CONNECTIONS 0
#define MAX_NR_L2CAP_CHANNELS 9  // Up to 7 RFCOMM channels, plus 2 for transient use.
#define MAX_NR_L2CAP_SERVICES 7
#define MAX_NR_RFCOMM_CHANNELS 8  // We can have up to 7 connections, so we keep one extra spare.
// We have one rfcomm service for the SPP profile.
#define MAX_NR_RFCOMM_MULTIPLEXERS 1
#define MAX_NR_RFCOMM_SERVICES 1
#define MAX_NR_SERVICE_RECORD_ITEMS 1
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_WHITELIST_ENTRIES 0
#define MAX_NR_LE_DEVICE_DB_ENTRIES 3
#define MAX_NR_SDP_CLIENTS 1

#endif  // _PLATFORM_EV3_BTSTACK_CONFIG_H_
