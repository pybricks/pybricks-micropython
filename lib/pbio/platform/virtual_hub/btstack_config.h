//
// btstack_config.h for libusb port
//
// Documentation: https://bluekitchen-gmbh.com/btstack/#how_to/
//

#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

// Port related features
#define HAVE_ASSERT
#define HAVE_BTSTACK_STDIN
#define HAVE_MALLOC
#define HAVE_POSIX_FILE_IO
#define HAVE_POSIX_TIME

#define ENABLE_LOG_INFO
#define ENABLE_LOG_ERROR
#define ENABLE_LOG_DEBUG

// BTstack features that can be enabled
#define ENABLE_ATT_DELAYED_RESPONSE
#define ENABLE_BLE
#define ENABLE_BTSTACK_STDIN_LOGGING
#define ENABLE_CLASSIC
#define ENABLE_CROSS_TRANSPORT_KEY_DERIVATION
#define ENABLE_HFP_WIDE_BAND_SPEECH
#define ENABLE_L2CAP_ENHANCED_RETRANSMISSION_MODE
#define ENABLE_LE_CENTRAL
#define ENABLE_L2CAP_LE_CREDIT_BASED_FLOW_CONTROL_MODE
#define ENABLE_LE_DATA_LENGTH_EXTENSION
#define ENABLE_LE_PERIPHERAL
#define ENABLE_LE_PRIVACY_ADDRESS_RESOLUTION
#define ENABLE_LE_SECURE_CONNECTIONS
#define ENABLE_LOG_ERROR
#define ENABLE_LOG_INFO
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS
#define ENABLE_PRINTF_HEXDUMP
#define ENABLE_SCO_OVER_HCI
#define ENABLE_SDP_DES_DUMP
#define ENABLE_SOFTWARE_AES128

// BTstack configuration. buffers, sizes, ...
#define HCI_ACL_PAYLOAD_SIZE (1691 + 4)
#define HCI_INCOMING_PRE_BUFFER_SIZE 14 // sizeof BNEP header, avoid memcpy

#define NVM_NUM_DEVICE_DB_ENTRIES      16
#define NVM_NUM_LINK_KEYS              16

// Mesh Configuration
#define ENABLE_MESH
#define ENABLE_MESH_ADV_BEARER
#define ENABLE_MESH_GATT_BEARER
#define ENABLE_MESH_PB_ADV
#define ENABLE_MESH_PB_GATT
#define ENABLE_MESH_PROVISIONER
#define ENABLE_MESH_PROXY_SERVER

#define MAX_NR_MESH_SUBNETS            2
#define MAX_NR_MESH_TRANSPORT_KEYS    16
#define MAX_NR_MESH_VIRTUAL_ADDRESSES 16

#define MAX_ATT_DB_SIZE 512
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 20
#define MAX_NR_HCI_CONNECTIONS 7 // CC2560/A limit.
#define MAX_NR_HFP_CONNECTIONS 0
// Up to 7 RFCOMM channels, plus 2 for transient use.
#define MAX_NR_L2CAP_CHANNELS 9
#define MAX_NR_L2CAP_SERVICES 7
// We can have up to 7 connections, so we keep one extra spare.
#define MAX_NR_RFCOMM_CHANNELS 8
// We have one rfcomm service for the SPP profile.
#define MAX_NR_RFCOMM_MULTIPLEXERS 1
#define MAX_NR_RFCOMM_SERVICES 1
#define MAX_NR_SERVICE_RECORD_ITEMS 1
#define MAX_NR_SM_LOOKUP_ENTRIES 3
#define MAX_NR_WHITELIST_ENTRIES 0
#define MAX_NR_LE_DEVICE_DB_ENTRIES 3
#define MAX_NR_SDP_CLIENTS 1

// allow for one NetKey update
#define MAX_NR_MESH_NETWORK_KEYS      (MAX_NR_MESH_SUBNETS + 1)

#endif
