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

// #define ENABLE_LOG_DEBUG
// #define ENABLE_LOG_ERROR
// #define ENABLE_LOG_INFO

// BTstack configuration. buffers, sizes, ...
// One full ATT PDU at PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE (515) plus the
// 4-byte L2CAP header, which is the most any link here negotiates. BTstack
// allocates this twice per HCI connection (ACL recombination buffer plus the
// per-connection ATT server request buffer), so it sets the static RAM cost
// of every extra connection. Kept the same on every platform so that links
// behave identically, even where there is RAM to spare.
#define HCI_ACL_PAYLOAD_SIZE (515 + 4)
#define MAX_ATT_DB_SIZE 512
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 0
// Host computer + HID gamepad + one per peer brick, plus headroom for a link
// that is on its way out while a replacement is paged. The baseband ceiling
// is 7 slaves regardless of what is configured here.
#define MAX_NR_HCI_CONNECTIONS 10
#define MAX_NR_GATT_CLIENTS 2
#define MAX_NR_HFP_CONNECTIONS 0
#define MAX_NR_HID_HOST_CONNECTIONS 1
// SDP client/server + HID control + HID interrupt channels, plus one peer
// channel per peer brick.
#define MAX_NR_L2CAP_CHANNELS 13
// SDP server + HID control + HID interrupt + peer services.
#define MAX_NR_L2CAP_SERVICES 5
#define MAX_NR_RFCOMM_CHANNELS 4
#define MAX_NR_RFCOMM_MULTIPLEXERS 1
#define MAX_NR_RFCOMM_SERVICES 1
// Device ID SDP record.
#define MAX_NR_SERVICE_RECORD_ITEMS 2
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_WHITELIST_ENTRIES 0
#define MAX_NR_LE_DEVICE_DB_ENTRIES 3

#endif  // _PLATFORM_EV3_BTSTACK_CONFIG_H_
