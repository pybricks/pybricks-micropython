// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbsys/light.h>
#include <pbio/protocol.h>
#include <pbio/util.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"

#include <pbio/button.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/common.h>
#include "pybricks/parameters.h"

#include <stdint.h>

#define LEGO_HUB_ID_POWERED_UP_REMOTE 0x42

#if PYBRICKS_HUB_MOVEHUB | PYBRICKS_HUB_CITYHUB | PYBRICKS_HUB_TECHNICHUB

#include <pbdrv/bluetooth.h>
#include <pbio/error.h>
#include <pbio/task.h>

#include <pybricks/util_pb/pb_error.h>

typedef struct {
    pbio_task_t task;
    uint8_t left[3];
    uint8_t right[3];
    uint8_t center;
    pbdrv_bluetooth_scan_and_connect_context_t context;
} pb_remote_t;

STATIC pb_remote_t pb_remote_singleton;

static void handle_notification(pbdrv_bluetooth_connection_t connection, const uint8_t *value, uint8_t size) {
    pb_remote_t *remote = &pb_remote_singleton;

    // 0x08 == H/W NetWork Commands, 0x02 == Connection Request
    // This message is meant for something else, but contains the center button state
    if (value[0] == 5 && value[2] == 0x08 && value[3] == 0x02) {
        remote->center = value[4];
    }
    // 0x45 == port value command
    else if (value[0] == 7 && value[2] == 0x45) {
        if (value[3] == 0) {
            memcpy(remote->left, &value[4], 3);
        } else if (value[3] == 1) {
            memcpy(remote->right, &value[4], 3);
        }
    }
}

STATIC void pb_remote_connect(mp_int_t timeout) {
    pb_remote_t *remote = &pb_remote_singleton;

    memset(remote, 0, sizeof(*remote));

    // TODO: check for busy

    pbdrv_bluetooth_set_notification_handler(handle_notification);
    pbdrv_bluetooth_scan_and_connect(&remote->task, &remote->context);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_int_t start = mp_hal_ticks_ms();
        while (timeout == -1 || mp_hal_ticks_ms() - start < timeout) {
            MICROPY_EVENT_POLL_HOOK
            if (remote->task.status == PBIO_SUCCESS) {
                nlr_pop();
                return;
            }
            if (remote->task.status != PBIO_ERROR_AGAIN) {
                nlr_pop();
                pb_assert(remote->task.status);
            }
        }
        mp_raise_OSError(MP_ETIMEDOUT);
        nlr_pop();
    } else {
        remote->task.cancel = true;
        while (remote->task.status == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP
        }
        nlr_jump(nlr.ret_val);
    }
}

void pb_type_Remote_cleanup(void) {
    pbdrv_bluetooth_disconnect_remote();
}

#elif PYBRICKS_HUB_PRIMEHUB

#include <btstack.h>

static btstack_packet_callback_registration_t hci_event_callback_registration;

#define ADV_IND                                0x00
#define ADV_DIRECT_IND                         0x01
#define ADV_SCAN_IND                           0x02
#define ADV_NONCONN_IND                        0x03
#define SCAN_RSP                               0x04

typedef enum {
    CON_STATE_NONE,
    CON_STATE_WAIT_ADV_IND,
    CON_STATE_WAIT_SCAN_RSP,
    CON_STATE_WAIT_CONNECT,
    CON_STATE_CONNECT_FAILED,
    CON_STATE_WAIT_DISCOVER_SERVICES,
    CON_STATE_WAIT_DISCOVER_CHARACTERISTICS,
    CON_STATE_WAIT_ENABLE_NOTIFICATIONS,
    CON_STATE_WAIT_WRITE_PORT_0_SETUP,
    CON_STATE_WAIT_WRITE_PORT_1_SETUP,
    CON_STATE_CONNECTED,
    CON_STATE_WAIT_DISCONNECT,
} con_state_t;

typedef enum {
    DISCONNECT_REASON_NONE,
    DISCONNECT_REASON_TIMEOUT,
    DISCONNECT_REASON_CONNECT_FAILED,
    DISCONNECT_REASON_DISCOVER_SERVICE_FAILED,
    DISCONNECT_REASON_DISCOVER_CHARACTERISTIC_FAILED,
    DISCONNECT_REASON_CONFIGURE_CHARACTERISTIC_FAILED,
    DISCONNECT_REASON_SEND_SUBSCRIBE_PORT_0_FAILED,
    DISCONNECT_REASON_SEND_SUBSCRIBE_PORT_1_FAILED,
} disconnect_reason_t;

typedef struct {
    uint16_t con_handle;
    con_state_t con_state;
    disconnect_reason_t disconnect_reason;
    uint8_t btstack_error;
    uint8_t left[3];
    uint8_t right[3];
    uint8_t center;
    bd_addr_t address;
    char name[20];
    uint8_t address_type;
    gatt_client_notification_t notification;
} pb_remote_t;

STATIC pb_remote_t pb_remote_singleton;

static gatt_client_service_t lwp3_service;
static gatt_client_characteristic_t lwp3_characteristic;

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    pb_remote_t *remote = &pb_remote_singleton;

    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            gatt_event_service_query_result_get_service(packet, &lwp3_service);
            break;

        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            gatt_event_characteristic_query_result_get_characteristic(packet, &lwp3_characteristic);
            break;

        case GATT_EVENT_QUERY_COMPLETE:
            if (remote->con_state == CON_STATE_WAIT_DISCOVER_SERVICES) {
                // TODO: remove cast on lwp3_characteristic_uuid after
                // https://github.com/bluekitchen/btstack/pull/359
                remote->btstack_error = gatt_client_discover_characteristics_for_service_by_uuid128(
                    handle_gatt_client_event, remote->con_handle, &lwp3_service, (uint8_t *)pbio_lwp3_hub_char_uuid);
                if (remote->btstack_error == ERROR_CODE_SUCCESS) {
                    remote->con_state = CON_STATE_WAIT_DISCOVER_CHARACTERISTICS;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(remote->con_handle);
                    remote->con_state = CON_STATE_WAIT_DISCONNECT;
                    remote->disconnect_reason = DISCONNECT_REASON_DISCOVER_CHARACTERISTIC_FAILED;
                }
            } else if (remote->con_state == CON_STATE_WAIT_DISCOVER_CHARACTERISTICS) {
                remote->btstack_error = gatt_client_write_client_characteristic_configuration(
                    handle_gatt_client_event, remote->con_handle, &lwp3_characteristic,
                    GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                if (remote->btstack_error == ERROR_CODE_SUCCESS) {
                    gatt_client_listen_for_characteristic_value_updates(
                        &remote->notification, handle_gatt_client_event, remote->con_handle, &lwp3_characteristic);
                    remote->con_state = CON_STATE_WAIT_ENABLE_NOTIFICATIONS;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(remote->con_handle);
                    remote->con_state = CON_STATE_WAIT_DISCONNECT;
                    remote->disconnect_reason = DISCONNECT_REASON_CONFIGURE_CHARACTERISTIC_FAILED;
                }
            } else if (remote->con_state == CON_STATE_WAIT_ENABLE_NOTIFICATIONS) {
                // 0x0a == length
                // 0x00 == local hub
                // 0x41 == Port Input Format Setup (Single)
                // 0x00 == Port ID - left buttons
                // 0x04 == mode - KEYSD
                // 0x00000001 == delta interval
                // 0x01 == enable notifications
                static const uint8_t subscribe_port_0[] = { 0x0a, 0x00, 0x41, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x01 };

                remote->btstack_error = gatt_client_write_value_of_characteristic(
                    handle_gatt_client_event, remote->con_handle, lwp3_characteristic.value_handle,
                    sizeof(subscribe_port_0), (uint8_t *)subscribe_port_0);
                if (remote->btstack_error == ERROR_CODE_SUCCESS) {
                    remote->con_state = CON_STATE_WAIT_WRITE_PORT_0_SETUP;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(remote->con_handle);
                    remote->con_state = CON_STATE_WAIT_DISCONNECT;
                    remote->disconnect_reason = DISCONNECT_REASON_SEND_SUBSCRIBE_PORT_0_FAILED;
                }
            } else if (remote->con_state == CON_STATE_WAIT_WRITE_PORT_0_SETUP) {
                // 0x0a == length
                // 0x00 == local hub
                // 0x41 == Port Input Format Setup (Single)
                // 0x01 == Port ID - right buttons
                // 0x04 == mode - KEYSD
                // 0x00000001 == delta interval
                // 0x01 == enable notifications
                static const uint8_t subscribe_port_1[] = { 0x0a, 0x00, 0x41, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00, 0x01 };

                remote->btstack_error = gatt_client_write_value_of_characteristic(
                    handle_gatt_client_event, remote->con_handle, lwp3_characteristic.value_handle,
                    sizeof(subscribe_port_1), (uint8_t *)subscribe_port_1);
                if (remote->btstack_error == ERROR_CODE_SUCCESS) {
                    remote->con_state = CON_STATE_WAIT_WRITE_PORT_1_SETUP;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(remote->con_handle);
                    remote->con_state = CON_STATE_WAIT_DISCONNECT;
                    remote->disconnect_reason = DISCONNECT_REASON_SEND_SUBSCRIBE_PORT_1_FAILED;
                }
            } else if (remote->con_state == CON_STATE_WAIT_WRITE_PORT_1_SETUP) {
                remote->con_state = CON_STATE_CONNECTED;
            }
            break;


        case GATT_EVENT_NOTIFICATION: {
            if (gatt_event_notification_get_handle(packet) != remote->con_handle) {
                break;
            }

            const uint8_t *value = gatt_event_notification_get_value(packet);

            // 0x08 == H/W NetWork Commands, 0x02 == Connection Request
            // This message is meant for something else, but contains the center button state
            if (value[0] == 5 && value[2] == 0x08 && value[3] == 0x02) {
                remote->center = value[4];
            }
            // 0x45 == port value command
            else if (value[0] == 7 && value[2] == 0x45) {
                if (value[3] == 0) {
                    memcpy(remote->left, &value[4], 3);
                } else if (value[3] == 1) {
                    memcpy(remote->right, &value[4], 3);
                }
            }

            break;
        }

        default:
            break;
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    pb_remote_t *remote = &pb_remote_singleton;

    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case GAP_EVENT_ADVERTISING_REPORT: {
            uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
            uint8_t data_length = gap_event_advertising_report_get_data_length(packet);
            const uint8_t *data = gap_event_advertising_report_get_data(packet);

            if (remote->con_state == CON_STATE_WAIT_ADV_IND) {
                // HACK: this is making major assumptions about how the advertising data
                // is laid out. So far LEGO devices seem consistent in this.
                // It is expected that the avertising data contains 3 values in
                // this order:
                // - Flags (0x01)
                // - Complete List of 128-bit Service Class UUIDs (0x07)
                // - Manufacturer Specific Data (0xFF)
                //   - LEGO System A/S (0x0397) + 6 bytes
                if (event_type == ADV_IND && data_length == 31
                    && pbio_uuid128_reverse_compare(&data[5], pbio_lwp3_hub_service_uuid)
                    && data[26] == LEGO_HUB_ID_POWERED_UP_REMOTE) {
                    gap_event_advertising_report_get_address(packet, remote->address);
                    remote->address_type = gap_event_advertising_report_get_address_type(packet);
                    remote->con_state = CON_STATE_WAIT_SCAN_RSP;
                }
            } else if (remote->con_state == CON_STATE_WAIT_SCAN_RSP) {
                bd_addr_t address;

                gap_event_advertising_report_get_address(packet, address);

                if (event_type == SCAN_RSP && data_length == 30 && bd_addr_cmp(address, remote->address) == 0) {
                    // TODO: verify name
                    if (data[1] == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) {
                        strncpy(remote->name, (const char *)&data[2], sizeof(remote->name));
                    }
                    gap_stop_scan();
                    remote->btstack_error = gap_connect(remote->address, remote->address_type);
                    if (remote->btstack_error == ERROR_CODE_SUCCESS) {
                        remote->con_state = CON_STATE_WAIT_CONNECT;
                    } else {
                        remote->con_state = CON_STATE_CONNECT_FAILED;
                    }
                }
            }

            break;
        }

        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                break;
            }

            // HCI_ROLE_MASTER means the connecting device is the peripheral and the hub is the central.
            if (hci_subevent_le_connection_complete_get_role(packet) != HCI_ROLE_MASTER) {
                break;
            }

            // If we aren't waiting for a connection, this must be a different connection.
            if (remote->con_state != CON_STATE_WAIT_CONNECT) {
                break;
            }

            remote->con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

            remote->btstack_error = gatt_client_discover_primary_services_by_uuid128(
                handle_gatt_client_event, remote->con_handle, pbio_lwp3_hub_service_uuid);
            if (remote->btstack_error == ERROR_CODE_SUCCESS) {
                remote->con_state = CON_STATE_WAIT_DISCOVER_SERVICES;
            } else {
                // configuration failed for some reason, so disconnect
                gap_disconnect(remote->con_handle);
                remote->con_state = CON_STATE_WAIT_DISCONNECT;
                remote->disconnect_reason = DISCONNECT_REASON_DISCOVER_SERVICE_FAILED;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            if (hci_event_disconnection_complete_get_connection_handle(packet) != remote->con_handle) {
                break;
            }

            gatt_client_stop_listening_for_characteristic_value_updates(&remote->notification);
            remote->con_handle = HCI_CON_HANDLE_INVALID;
            remote->con_state = CON_STATE_NONE;

            // TODO: we should probably try to reconnect
            break;

        default:
            break;
    }
}

STATIC void pb_remote_connect(mp_int_t timeout) {
    pb_remote_t *remote = &pb_remote_singleton;

    if (remote->con_state != CON_STATE_NONE) {
        mp_raise_OSError(MP_EBUSY);
    }

    memset(remote, 0, sizeof(*remote));

    if (hci_event_callback_registration.callback == NULL) {
        hci_event_callback_registration.callback = &packet_handler;
        hci_add_event_handler(&hci_event_callback_registration);
    }

    // active scanning to get scan response data.
    // scan interval: 48 * 0.625ms = 30ms
    gap_set_scan_params(1, 0x30, 0x30, 0);
    gap_start_scan();
    remote->con_handle = HCI_CON_HANDLE_INVALID;
    remote->con_state = CON_STATE_WAIT_ADV_IND;

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_int_t start = mp_hal_ticks_ms();
        while (timeout == -1 || mp_hal_ticks_ms() - start < timeout) {
            MICROPY_EVENT_POLL_HOOK
            if (remote->con_state == CON_STATE_CONNECTED) {
                nlr_pop();
                return;
            }
            if (remote->con_state == CON_STATE_CONNECT_FAILED) {
                mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("failed to connect"));
            }
            if (remote->con_state == CON_STATE_NONE) {
                mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("disconnected: %d %d"),
                    remote->disconnect_reason, remote->btstack_error);
            }
        }
        mp_raise_OSError(MP_ETIMEDOUT);
        nlr_pop();
    } else {
        if (remote->con_state == CON_STATE_WAIT_ADV_IND || remote->con_state == CON_STATE_WAIT_SCAN_RSP) {
            gap_stop_scan();
        } else if (remote->con_state == CON_STATE_WAIT_CONNECT) {
            gap_connect_cancel();
        } else if (remote->con_handle != HCI_CON_HANDLE_INVALID) {
            gap_disconnect(remote->con_handle);
        }
        remote->con_state = CON_STATE_NONE;
        nlr_jump(nlr.ret_val);
    }
}

void pb_type_Remote_cleanup(void) {
    if (pb_remote_singleton.con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(pb_remote_singleton.con_handle);
    }
}

#endif // PYBRICKS_HUB_PRIMEHUB


STATIC pbio_error_t remote_button_is_pressed(pbio_button_flags_t *pressed) {

    pb_remote_t *remote = &pb_remote_singleton;

    #if PYBRICKS_HUB_PRIMEHUB
    if (remote->con_state != CON_STATE_CONNECTED)
    #else
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_HANDSET))
    #endif
    {
        mp_raise_OSError(MP_ENODEV);
    }

    *pressed = 0;

    if (remote->left[0]) {
        *pressed |= PBIO_BUTTON_LEFT_UP;
    }
    if (remote->left[1]) {
        *pressed |= PBIO_BUTTON_LEFT;
    }
    if (remote->left[2]) {
        *pressed |= PBIO_BUTTON_LEFT_DOWN;
    }
    if (remote->right[0]) {
        *pressed |= PBIO_BUTTON_RIGHT_UP;
    }
    if (remote->right[1]) {
        *pressed |= PBIO_BUTTON_RIGHT;
    }
    if (remote->right[2]) {
        *pressed |= PBIO_BUTTON_RIGHT_DOWN;
    }
    if (remote->center) {
        *pressed |= PBIO_BUTTON_CENTER;
    }
    return PBIO_SUCCESS;
}

typedef struct _pb_type_pupdevices_Remote_obj_t {
    mp_obj_base_t base;
    mp_obj_t buttons;
} pb_type_pupdevices_Remote_obj_t;

STATIC const pb_obj_enum_member_t *remote_buttons[] = {
    &pb_Button_LEFT_MINUS_obj,
    &pb_Button_RIGHT_MINUS_obj,
    &pb_Button_LEFT_obj,
    &pb_Button_CENTER_obj,
    &pb_Button_RIGHT_obj,
    &pb_Button_LEFT_PLUS_obj,
    &pb_Button_RIGHT_PLUS_obj
};

STATIC mp_obj_t pb_type_pupdevices_Remote_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_type_pupdevices_Remote_obj_t *self = m_new_obj(pb_type_pupdevices_Remote_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    if (name_in != mp_const_none) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("filter by name is not implemented"));
    }

    mp_int_t timeout = timeout_in == mp_const_none? -1 : pb_obj_get_positive_int(timeout_in);
    pb_remote_connect(timeout);

    self->buttons = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(remote_buttons), remote_buttons, remote_button_is_pressed);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t pb_type_pupdevices_Remote_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_ATTRIBUTE_OFFSET(pb_type_pupdevices_Remote_obj_t, buttons) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_pupdevices_Remote_locals_dict, pb_type_pupdevices_Remote_locals_dict_table);

const mp_obj_type_t pb_type_pupdevices_Remote = {
    { &mp_type_type },
    .name = PYBRICKS_HUB_CLASS_NAME,
    .make_new = pb_type_pupdevices_Remote_make_new,
    .locals_dict = (mp_obj_dict_t *)&pb_type_pupdevices_Remote_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
