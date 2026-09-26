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
// Sized by the largest link, which here is Classic RFCOMM to a host computer:
// one COBS-encoded Pybricks packet (521 bytes, being the 512-byte host event
// message plus its endpoint byte, the COBS overhead and the frame delimiter),
// plus the 5-byte RFCOMM frame overhead and the 4-byte L2CAP header. Anything
// less splits every large packet over two frames for no good reason.
//
// BLE needs less: one full ATT PDU at PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE
// (515) plus the same 4-byte L2CAP header. The LE MTU no longer follows this
// number, since pbdrv_bluetooth_init() pins it to the platform maximum.
//
// BTstack allocates this twice per HCI connection (ACL recombination buffer
// plus the per-connection ATT server request buffer), so it sets the static
// RAM cost of every extra connection. Platforms without a Classic host
// connection stay at the smaller BLE-sized value. bluetooth_btstack.c
// static-asserts that a whole packet fits in one RFCOMM frame.
#define HCI_ACL_PAYLOAD_SIZE (521 + 5 + 4)
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

// allow for one NetKey update
#define MAX_NR_MESH_NETWORK_KEYS      (MAX_NR_MESH_SUBNETS + 1)

#endif
