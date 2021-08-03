// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// LEGO Wireless Protocol v3 (LWP3)
// https://lego.github.io/lego-ble-wireless-protocol-docs/

#ifndef _LEGO_LWP3_H_
#define _LEGO_LWP3_H_

// https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/
#define LWP3_LEGO_COMPANY_ID 0x0397

#define LWP3_HUB_SYSTEM(id) (id << 5)

typedef enum {
    LWP3_HUB_SYSTEM_WEDO2       = LWP3_HUB_SYSTEM(0),
    LWP3_HUB_SYSTEM_DUPLO       = LWP3_HUB_SYSTEM(1),
    LWP3_HUB_SYSTEM_SYSTEM      = LWP3_HUB_SYSTEM(2),
    LWP3_HUB_SYSTEM_SYSTEM_     = LWP3_HUB_SYSTEM(3),
    LWP3_HUB_SYSTEM_TECHNIC     = LWP3_HUB_SYSTEM(4),
} lwp3_hub_system_t;

#define LWP3_HUB_KIND(system, device) (system | device)

typedef enum {
    LWP3_HUB_KIND_WEDO2             = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_WEDO2, 0),

    LWP3_HUB_KIND_DUPLO_TRAIN       = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_DUPLO, 0),

    LWP3_HUB_KIND_BOOST             = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 0),
    LWP3_HUB_KIND_SYSTEM_2IO        = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 1),
    LWP3_HUB_KIND_HANDSET           = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 2),
    LWP3_HUB_KIND_MARIO             = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_SYSTEM, 3),

    LWP3_HUB_KIND_TECHNIC_MEDIUM    = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_TECHNIC, 0),
    LWP3_HUB_KIND_TECHNIC_LARGE     = LWP3_HUB_KIND(LWP3_HUB_SYSTEM_TECHNIC, 1),
} lwp3_hub_kind_t;

typedef enum {
    LWP3_MSG_TYPE_HUB_PROPERTIES            = 0x01,
    LWP3_MSG_TYPE_HUB_ACTIONS               = 0x02,
    LWP3_MSG_TYPE_HUB_ALERTS                = 0x03,
    LWP3_MSG_TYPE_HUB_IO                    = 0x04,
    LWP3_MSG_TYPE_ERROR                     = 0x05,
    LWP3_MSG_TYPE_HW_NET_CMDS               = 0x08,
    LWP3_MSG_TYPE_FW_BOOT_MODE              = 0x10,
    LWP3_MSG_TYPE_FW_LOCK                   = 0x11,
    LWP3_MSG_TYPE_FW_LOCK_STATUS_REQ        = 0x12,
    LWP3_MSG_TYPE_FW_LOCK_STATUS            = 0x13,
    LWP3_MSG_TYPE_PORT_INFO_REQ             = 0x21,
    LWP3_MSG_TYPE_PORT_MODE_INFO_REQ        = 0x22,
    LWP3_MSG_TYPE_PORT_MODE_SETUP           = 0x41,
    LWP3_MSG_TYPE_PORT_MODE_COMBO_SETUP     = 0x42,
    LWP3_MSG_TYPE_PORT_INFO                 = 0x43,
    LWP3_MSG_TYPE_PORT_MODE_INFO            = 0x44,
    LWP3_MSG_TYPE_PORT_VALUE                = 0x45,
    LWP3_MSG_TYPE_PORT_COMBO_VALUE          = 0x46,
    LWP3_MSG_TYPE_PORT_MODE                 = 0x47,
    LWP3_MSG_TYPE_PORT_MODE_COMBO           = 0x48,
    LWP3_MSG_TYPE_VIRT_PORT_SETUP           = 0x61,
    LWP3_MSG_TYPE_OUT_PORT_CMD              = 0x81,
    LWP3_MSG_TYPE_OUT_PORT_CMD_RSP          = 0x82,
} lwp3_msg_type_t;

typedef enum {
    LWP3_HUB_PROPERTY_NAME                  = 0x01,
    LWP3_HUB_PROPERTY_BUTTON                = 0x02,
    LWP3_HUB_PROPERTY_FW_VERSION            = 0x03,
    LWP3_HUB_PROPERTY_HW_VERSION            = 0x04,
    LWP3_HUB_PROPERTY_RSSI                  = 0x05,
    LWP3_HUB_PROPERTY_BATTERY_VOLTAGE       = 0x06,
    LWP3_HUB_PROPERTY_BATTERY_TYPE          = 0x07,
    LWP3_HUB_PROPERTY_MFG_NAME              = 0x08,
    LWP3_HUB_PROPERTY_RADIO_FW_VERSION      = 0x09,
    LWP3_HUB_PROPERTY_PROTOCOL_VERSION      = 0x0A,
    LWP3_HUB_PROPERTY_SYSTEM_TYPE_ID        = 0x0B,
    LWP3_HUB_PROPERTY_HW_NET_ID             = 0x0C,
    LWP3_HUB_PROPERTY_PRIMARY_ADDR          = 0x0D,
    LWP3_HUB_PROPERTY_SECONDARY_ADDR        = 0x0E,
    LWP3_HUB_PROPERTY_HW_NET_FAMILY         = 0x0F,
} lwp3_hub_property_t;

/** max size for ::LWP3_HUB_PROPERTY_NAME payload */
#define LWP3_MAX_HUB_PROPERTY_NAME_SIZE 14
typedef enum {
    LWP3_HUB_PROPERTY_OP_SET                = 0x01,
    LWP3_HUB_PROPERTY_OP_ENABLE             = 0x02,
    LWP3_HUB_PROPERTY_OP_DISABLE            = 0x03,
    LWP3_HUB_PROPERTY_OP_RESET              = 0x04,
    LWP3_HUB_PROPERTY_OP_REQUEST            = 0x05,
    LWP3_HUB_PROPERTY_OP_VALUE              = 0x06,
} lwp3_hub_property_op_t;

typedef enum {
    LWP3_HW_NET_CMD_CONNECTION_REQ          = 0x02,
    LWP3_HW_NET_CMD_FAMILY_REQ              = 0x03,
    LWP3_HW_NET_CMD_SET_FAMILY              = 0x04,
    LWP3_HW_NET_CMD_JOIN_DENIED             = 0x05,
    LWP3_HW_NET_CMD_GET_FAMILY              = 0x06,
    LWP3_HW_NET_CMD_FAMILY                  = 0x07,
    LWP3_HW_NET_CMD_GET_SUB_FAMILY          = 0x08,
    LWP3_HW_NET_CMD_SUB_FAMILY              = 0x09,
    LWP3_HW_NET_CMD_SET_SUB_FAMILY          = 0x0A,
    LWP3_HW_NET_CMD_GET_EXT_FAMILY          = 0x0B,
    LWP3_HW_NET_CMD_EXT_FAMILY              = 0x0C,
    LWP3_HW_NET_CMD_SET_EXT_FAMILY          = 0x0D,
    LWP3_HW_NET_CMD_RESET_LONG_PRESS        = 0x0E,
} lwp3_hw_net_cmd_t;

typedef enum {
    LWP3_STARTUP_BUFFER                     = 0x0,
    LWP3_STARTUP_IMMEDIATE                  = 0x1,
} lwp3_startup_t;

typedef enum {
    LWP3_COMPLETION_NO_ACTION               = 0x0,
    LWP3_COMPLETION_FEEDBACK                = 0x1,
} lwp3_completion_t;

typedef enum {
    LWP3_OUTPUT_CMD_START_POWER                 = 0x01,
    LWP3_OUTPUT_CMD_START_POWER_2               = 0x02,
    LWP3_OUTPUT_CMD_SET_ACC_TIME                = 0x05,
    LWP3_OUTPUT_CMD_SET_DEC_TIME                = 0x06,
    LWP3_OUTPUT_CMD_START_SPEED                 = 0x07,
    LWP3_OUTPUT_CMD_START_SPEED_2               = 0x08,
    LWP3_OUTPUT_CMD_START_SPEED_FOR_TIME        = 0x09,
    LWP3_OUTPUT_CMD_START_SPEED_FOR_TIME_2      = 0x0A,
    LWP3_OUTPUT_CMD_START_SPEED_FOR_DEGREES     = 0x0B,
    LWP3_OUTPUT_CMD_START_SPEED_FOR_DEGREES_2   = 0x0C,
    LWP3_OUTPUT_CMD_GOTO_ABS_POS                = 0x0D,
    LWP3_OUTPUT_CMD_GOTO_ABS_POS_2              = 0x0E,
    LWP3_OUTPUT_CMD_PRESET_ENCODER              = 0x13,
    LWP3_OUTPUT_CMD_PRESET_ENCODER_2            = 0x14,
    LWP3_OUTPUT_CMD_WRITE_DIRECT                = 0x50,
    LWP3_OUTPUT_CMD_WRITE_DIRECT_MODE_DATA      = 0x51,
} lwp3_output_cmd_t;

#endif // _LEGO_LWP3_H_
