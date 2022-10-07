// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

// Bluetooth driver using BlueKitchen BTStack.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK

#include <ble/gatt-service/device_information_service_server.h>
#include <ble/gatt-service/nordic_spp_service_server.h>
#include <btstack.h>
#include <contiki.h>
#include <lego_lwp3.h>

#include <pbdrv/bluetooth.h>
#include <pbio/protocol.h>
#include <pbio/task.h>
#include <pbio/version.h>

#include "bluetooth_btstack_run_loop_contiki.h"
#include "bluetooth_btstack.h"
#include "genhdr/pybricks_service.h"
#include "pybricks_service_server.h"

#ifdef PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_KIND
#define HUB_KIND PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_KIND
#else
#error "PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_KIND is required"
#endif

// location of product variant in bootloader flash memory of Technic Large hubs
#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_VARIANT_ADDR
#define HUB_VARIANT (*(const uint16_t *)PBDRV_CONFIG_BLUETOOTH_BTSTACK_HUB_VARIANT_ADDR)
#else
#define HUB_VARIANT 0x0000
#endif

// these aren't in btstack for some reason
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
    gatt_client_notification_t notification;
    uint16_t con_handle;
    con_state_t con_state;
    disconnect_reason_t disconnect_reason;
    gatt_client_service_t lwp3_service;
    gatt_client_characteristic_t lwp3_char;
    uint8_t btstack_error;
    uint8_t address_type;
    bd_addr_t address;
    lwp3_hub_kind_t hub_kind;
    char name[20];
} pup_handset_t;

// hub name goes in special section so that it can be modified when flashing firmware
#if !PBIO_TEST_BUILD
__attribute__((section(".name")))
#endif
char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

LIST(task_queue);
static hci_con_handle_t le_con_handle = HCI_CON_HANDLE_INVALID;
static hci_con_handle_t pybricks_con_handle = HCI_CON_HANDLE_INVALID;
static hci_con_handle_t uart_con_handle = HCI_CON_HANDLE_INVALID;
static pbdrv_bluetooth_on_event_t bluetooth_on_event;
static pbdrv_bluetooth_receive_handler_t receive_handler;
static pbdrv_bluetooth_receive_handler_t notification_handler;
static pup_handset_t handset;
static uint8_t *event_packet;
static const pbdrv_bluetooth_btstack_platform_data_t *pdata = &pbdrv_bluetooth_btstack_platform_data;

// note on baud rate: with a 48MHz clock, 3000000 baud is the highest we can
// go with LL_USART_OVERSAMPLING_16. With LL_USART_OVERSAMPLING_8 we could go
// to 4000000, which is the max rating of the CC2564C.

static const hci_transport_config_uart_t config = {
    .type = HCI_TRANSPORT_CONFIG_UART,
    .baudrate_init = 115200,
    .baudrate_main = 3000000,
    .flowcontrol = 1,
    .device_name = NULL,
};

/**
 * Converts BTStack error to most appropriate PBIO error.
 * @param [in]  status      The BTStack error
 * @return                  The PBIO error.
 */
static pbio_error_t att_error_to_pbio_error(uint8_t status) {
    switch (status) {
        case ATT_ERROR_SUCCESS:
            return PBIO_SUCCESS;
        case ATT_ERROR_HCI_DISCONNECT_RECEIVED:
            return PBIO_ERROR_NO_DEV;
        case ATT_ERROR_TIMEOUT:
            return PBIO_ERROR_TIMEDOUT;
        default:
            return PBIO_ERROR_FAILED;
    }
}

static void pybricks_can_send(void *context) {
    pbdrv_bluetooth_send_context_t *send = context;

    pybricks_service_server_send(pybricks_con_handle, send->data, send->size);
    send->done();
}

static pbio_pybricks_error_t pybricks_data_received(hci_con_handle_t tx_con_handle, const uint8_t *data, uint16_t size) {
    if (receive_handler) {
        return receive_handler(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS, data, size);
    }

    return ATT_ERROR_UNLIKELY_ERROR;
}

static void pybricks_configured(hci_con_handle_t tx_con_handle, uint16_t value) {
    pybricks_con_handle = value ? tx_con_handle : HCI_CON_HANDLE_INVALID;
}

static void nordic_can_send(void *context) {
    pbdrv_bluetooth_send_context_t *send = context;

    nordic_spp_service_server_send(uart_con_handle, send->data, send->size);
    send->done();
}

static void nordic_spp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) {
                break;
            }

            switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED:
                    uart_con_handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                    break;
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED:
                    uart_con_handle = HCI_CON_HANDLE_INVALID;
                    break;
                default:
                    break;
            }

            break;
        case RFCOMM_DATA_PACKET:
            if (receive_handler) {
                receive_handler(PBDRV_BLUETOOTH_CONNECTION_UART, packet, size);
            }
            break;
        default:
            break;
    }
}

// REVISIT: does this need to be separate from packet_handler()?
// currently, this function just handles the Powered Up handset control.
static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            gatt_event_service_query_result_get_service(packet, &handset.lwp3_service);
            break;

        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            gatt_event_characteristic_query_result_get_characteristic(packet, &handset.lwp3_char);
            break;

        case GATT_EVENT_QUERY_COMPLETE:
            if (handset.con_state == CON_STATE_WAIT_DISCOVER_SERVICES) {
                // TODO: remove cast on lwp3_characteristic_uuid after
                // https://github.com/bluekitchen/btstack/pull/359
                handset.btstack_error = gatt_client_discover_characteristics_for_service_by_uuid128(
                    handle_gatt_client_event, handset.con_handle, &handset.lwp3_service, (uint8_t *)pbio_lwp3_hub_char_uuid);
                if (handset.btstack_error == ERROR_CODE_SUCCESS) {
                    handset.con_state = CON_STATE_WAIT_DISCOVER_CHARACTERISTICS;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(handset.con_handle);
                    handset.con_state = CON_STATE_WAIT_DISCONNECT;
                    handset.disconnect_reason = DISCONNECT_REASON_DISCOVER_CHARACTERISTIC_FAILED;
                }
            } else if (handset.con_state == CON_STATE_WAIT_DISCOVER_CHARACTERISTICS) {
                handset.btstack_error = gatt_client_write_client_characteristic_configuration(
                    handle_gatt_client_event, handset.con_handle, &handset.lwp3_char,
                    GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                if (handset.btstack_error == ERROR_CODE_SUCCESS) {
                    gatt_client_listen_for_characteristic_value_updates(
                        &handset.notification, handle_gatt_client_event, handset.con_handle, &handset.lwp3_char);
                    handset.con_state = CON_STATE_WAIT_ENABLE_NOTIFICATIONS;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(handset.con_handle);
                    handset.con_state = CON_STATE_WAIT_DISCONNECT;
                    handset.disconnect_reason = DISCONNECT_REASON_CONFIGURE_CHARACTERISTIC_FAILED;
                }
            } else if (handset.con_state == CON_STATE_WAIT_ENABLE_NOTIFICATIONS) {
                handset.con_state = CON_STATE_CONNECTED;
            }
            break;

        case GATT_EVENT_NOTIFICATION: {
            if (gatt_event_notification_get_handle(packet) != handset.con_handle) {
                break;
            }

            if (notification_handler != NULL) {
                uint16_t length = gatt_event_notification_get_value_length(packet);
                const uint8_t *value = gatt_event_notification_get_value(packet);
                notification_handler(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_LWP3, value, length);
            }
            break;
        }

        default:
            break;
    }
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                break;
            }

            // HCI_ROLE_SLAVE means the connecting device is the central and the hub is the peripheral
            // HCI_ROLE_MASTER means the connecting device is the peripheral and the hub is the central.
            if (hci_subevent_le_connection_complete_get_role(packet) == HCI_ROLE_SLAVE) {
                le_con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
            } else {
                // If we aren't waiting for a handset connection, this must be a different connection.
                if (handset.con_state != CON_STATE_WAIT_CONNECT) {
                    break;
                }

                handset.con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                handset.btstack_error = gatt_client_discover_primary_services_by_uuid128(
                    handle_gatt_client_event, handset.con_handle, pbio_lwp3_hub_service_uuid);
                if (handset.btstack_error == ERROR_CODE_SUCCESS) {
                    handset.con_state = CON_STATE_WAIT_DISCOVER_SERVICES;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(handset.con_handle);
                    handset.con_state = CON_STATE_WAIT_DISCONNECT;
                    handset.disconnect_reason = DISCONNECT_REASON_DISCOVER_SERVICE_FAILED;
                }
            }

            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            if (hci_event_disconnection_complete_get_connection_handle(packet) == le_con_handle) {
                le_con_handle = HCI_CON_HANDLE_INVALID;
                pybricks_con_handle = HCI_CON_HANDLE_INVALID;
                uart_con_handle = HCI_CON_HANDLE_INVALID;
            } else if (hci_event_disconnection_complete_get_connection_handle(packet) == handset.con_handle) {
                gatt_client_stop_listening_for_characteristic_value_updates(&handset.notification);
                handset.con_handle = HCI_CON_HANDLE_INVALID;
                handset.con_state = CON_STATE_NONE;
            }

            break;

        case GAP_EVENT_ADVERTISING_REPORT: {
            uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
            uint8_t data_length = gap_event_advertising_report_get_data_length(packet);
            const uint8_t *data = gap_event_advertising_report_get_data(packet);
            bd_addr_t address;

            gap_event_advertising_report_get_address(packet, address);

            if (handset.con_state == CON_STATE_WAIT_ADV_IND) {
                // HACK: this is making major assumptions about how the advertising data
                // is laid out. So far LEGO devices seem consistent in this.
                // It is expected that the advertising data contains 3 values in
                // this order:
                // - Flags (0x01)
                // - Complete List of 128-bit Service Class UUIDs (0x07)
                // - Manufacturer Specific Data (0xFF)
                //   - LEGO System A/S (0x0397) + 6 bytes
                if (event_type == ADV_IND && data_length == 31
                    && pbio_uuid128_reverse_compare(&data[5], pbio_lwp3_hub_service_uuid)
                    && data[26] == handset.hub_kind) {

                    if (memcmp(address, handset.address, 6) == 0) {
                        // This was the same device as last time. If the scan response
                        // didn't match before, it probably won't match now and we
                        // should try a different device.
                        break;
                    }

                    memcpy(handset.address, address, sizeof(bd_addr_t));
                    handset.address_type = gap_event_advertising_report_get_address_type(packet);
                    handset.con_state = CON_STATE_WAIT_SCAN_RSP;
                }
            } else if (handset.con_state == CON_STATE_WAIT_SCAN_RSP) {
                // REVISIT: for now it is assumed that the saved Bluetooth address compare
                //          is a sufficient check to check that scan response is from LWP3 device
                // PREVIOUSLY: extra check: data_length == 30 for HANDSET
                //                          data_length == 27 for MARIO
                //                          data_length == 20 for SYSTEM_2IO ... etc
                if (event_type == SCAN_RSP && bd_addr_cmp(address, handset.address) == 0) {
                    if (data[1] == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) {
                        // if the name was passed in from the caller, then filter on name
                        if (handset.name[0] != '\0' && strncmp(handset.name, (char *)&data[2], sizeof(handset.name)) != 0) {
                            handset.con_state = CON_STATE_WAIT_ADV_IND;
                            break;
                        }

                        memcpy(handset.name, &data[2], sizeof(handset.name));
                    }

                    gap_stop_scan();
                    handset.btstack_error = gap_connect(handset.address, handset.address_type);

                    if (handset.btstack_error == ERROR_CODE_SUCCESS) {
                        handset.con_state = CON_STATE_WAIT_CONNECT;
                    } else {
                        handset.con_state = CON_STATE_CONNECT_FAILED;
                    }
                }
            }

            break;
        }

        default:
            break;
    }

    event_packet = packet;
    pbio_task_queue_run_once(task_queue);
    event_packet = NULL;

    if (bluetooth_on_event) {
        bluetooth_on_event();
    }
}

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    uint16_t att_value_len;

    switch (attribute_handle) {
        case ATT_CHARACTERISTIC_GAP_DEVICE_NAME_01_VALUE_HANDLE:
            att_value_len = strlen(pbdrv_bluetooth_hub_name);
            if (buffer) {
                memcpy(buffer, pbdrv_bluetooth_hub_name, att_value_len);
            }
            return att_value_len;

        default:
            return 0;
    }
}

void pbdrv_bluetooth_init(void) {
    static btstack_packet_callback_registration_t hci_event_callback_registration;

    // don't need to init the whole struct, so doing this here
    handset.con_handle = HCI_CON_HANDLE_INVALID;

    btstack_memory_init();
    btstack_run_loop_init(pbdrv_bluetooth_btstack_run_loop_contiki_get_instance());

    hci_init(hci_transport_h4_instance(pdata->uart_block_instance()), &config);
    hci_set_chipset(pdata->chipset_instance());
    hci_set_control(pdata->control_instance());

    // REVISIT: do we need to call btstack_chipset_cc256x_set_power() or btstack_chipset_cc256x_set_power_vector()?

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();

    // setup LE device DB
    le_device_db_init();

    // setup security manager
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_er((uint8_t *)pdata->er_key);
    sm_set_ir((uint8_t *)pdata->ir_key);

    // GATT Client setup
    gatt_client_init();

    // setup ATT server
    att_server_init(profile_data, att_read_callback, NULL);

    device_information_service_server_init();
    device_information_service_server_set_firmware_revision(PBIO_VERSION_STR);
    device_information_service_server_set_software_revision(PBIO_PROTOCOL_VERSION_STR);
    device_information_service_server_set_pnp_id(0x01, LWP3_LEGO_COMPANY_ID, HUB_KIND, HUB_VARIANT);

    pybricks_service_server_init(pybricks_data_received, pybricks_configured);
    nordic_spp_service_server_init(nordic_spp_packet_handler);
}

void pbdrv_bluetooth_power_on(bool on) {
    hci_power_control(on ? HCI_POWER_ON : HCI_POWER_OFF);
}

bool pbdrv_bluetooth_is_ready(void) {
    return hci_get_state() != HCI_STATE_OFF;
}

const char *pbdrv_bluetooth_get_hub_name(void) {
    return pbdrv_bluetooth_hub_name;
}

static void init_advertising_data(void) {
    bd_addr_t null_addr = { };
    gap_advertisements_set_params(0x0030, 0x0030, 0x00, 0x00, null_addr, 0x07, 0x00);

    static const uint8_t adv_data[] = {
        // Flags general discoverable, BR/EDR not supported
        2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
        // Pybricks service
        17, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS,
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89, 0xda, 0x46, 0x80, 0x82, 0x01, 0x00, 0xf5, 0xc5,
        // Tx Power
        2, BLUETOOTH_DATA_TYPE_TX_POWER_LEVEL, 0,
    };

    _Static_assert(sizeof(adv_data) <= 31, "31 octet max");

    gap_advertisements_set_data(sizeof(adv_data), (uint8_t *)adv_data);

    static uint8_t scan_resp_data[31] = {
        10, BLUETOOTH_DATA_TYPE_SERVICE_DATA,
        // used to identify which hub - Device Information Service (DIS).
        // 0x2A50 - service UUID - PnP ID characteristic UUID
        // 0x01 - Vendor ID Source Field - Bluetooth SIG-assigned ID
        // 0x0397 - Vendor ID Field - LEGO company identifier
        // 0x00XX - Product ID Field - hub kind
        // 0x00XX - Product Version Field - product variant
        0x50, 0x2a, 0x01, 0x97, 0x03, HUB_KIND, 0x00, 0x00, 0x00,
        0, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    };

    scan_resp_data[9] = HUB_VARIANT;

    uint8_t hub_name_len = strlen(pbdrv_bluetooth_hub_name);
    scan_resp_data[11] = hub_name_len + 1;
    memcpy(&scan_resp_data[13], pbdrv_bluetooth_hub_name, hub_name_len);
    _Static_assert(13 + sizeof(pbdrv_bluetooth_hub_name) - 1 <= 31, "scan response is 31 octet max");

    gap_scan_response_set_data(13 + hub_name_len, scan_resp_data);
}

void pbdrv_bluetooth_start_advertising(void) {
    init_advertising_data();
    gap_advertisements_enable(true);
}

void pbdrv_bluetooth_stop_advertising(void) {
    gap_advertisements_enable(false);
}

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    if (connection == PBDRV_BLUETOOTH_CONNECTION_LE && le_con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS && pybricks_con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_UART && uart_con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_LWP3 && handset.con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }

    return false;
}

void pbdrv_bluetooth_set_on_event(pbdrv_bluetooth_on_event_t on_event) {
    bluetooth_on_event = on_event;
}

void pbdrv_bluetooth_send(pbdrv_bluetooth_send_context_t *context) {
    static btstack_context_callback_registration_t send_request;

    send_request.context = context;

    if (context->connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS) {
        send_request.callback = &pybricks_can_send;
        pybricks_service_server_request_can_send_now(&send_request, pybricks_con_handle);
    } else if (context->connection == PBDRV_BLUETOOTH_CONNECTION_UART) {
        send_request.callback = &nordic_can_send;
        nordic_spp_service_server_request_can_send_now(&send_request, uart_con_handle);
    }
}

void pbdrv_bluetooth_set_receive_handler(pbdrv_bluetooth_receive_handler_t handler) {
    receive_handler = handler;
}

void pbdrv_bluetooth_set_notification_handler(pbdrv_bluetooth_receive_handler_t handler) {
    notification_handler = handler;
}

static PT_THREAD(scan_and_connect_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_scan_and_connect_context_t *context = task->context;

    PT_BEGIN(pt);

    memset(&handset, 0, sizeof(handset));
    handset.con_handle = HCI_CON_HANDLE_INVALID;
    memcpy(handset.name, context->name, sizeof(handset.name));
    handset.hub_kind = context->hub_kind;

    // active scanning to get scan response data.
    // scan interval: 48 * 0.625ms = 30ms
    gap_set_scan_params(1, 0x30, 0x30, 0);
    gap_start_scan();
    handset.con_state = CON_STATE_WAIT_ADV_IND;

    PT_WAIT_UNTIL(pt, ({
        if (task->cancel) {
            goto cancel;
        }

        handset.con_state == CON_STATE_CONNECTED;
    }));

    // TODO: set task->status to error on CON_STATE_CONNECT_FAILED

    // REVISIT: probably want to make a generic connection handle data structure
    // that includes handle, name, address, etc.
    memcpy(context->name, handset.name, sizeof(context->name));

    task->status = PBIO_SUCCESS;
    PT_EXIT(pt);

cancel:
    if (handset.con_state == CON_STATE_WAIT_ADV_IND || handset.con_state == CON_STATE_WAIT_SCAN_RSP) {
        gap_stop_scan();
    } else if (handset.con_state == CON_STATE_WAIT_CONNECT) {
        gap_connect_cancel();
    } else if (handset.con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(handset.con_handle);
    }
    handset.con_state = CON_STATE_NONE;
    task->status = PBIO_ERROR_CANCELED;
    PT_END(pt);
}

void pbdrv_bluetooth_scan_and_connect(pbio_task_t *task, pbdrv_bluetooth_scan_and_connect_context_t *context) {
    pbio_task_init(task, scan_and_connect_task, context);
    pbio_task_queue_add(task_queue, task);
}

static PT_THREAD(write_remote_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_value_t *value = task->context;

    PT_BEGIN(pt);

    // TODO: Make connection handle and value handle part of pbdrv_bluetooth_value_t
    // to allow for simultaneous connections.

    uint8_t err = gatt_client_write_value_of_characteristic(packet_handler,
        handset.con_handle, handset.lwp3_char.value_handle, value->size, value->data);

    if (err != ERROR_CODE_SUCCESS) {
        task->status = PBIO_ERROR_FAILED;
        PT_EXIT(pt);
    }

    // NB: Value buffer must remain valid until GATT_EVENT_QUERY_COMPLETE, so
    // this wait is not cancelable.
    PT_WAIT_UNTIL(pt, ({
        if (handset.con_handle == HCI_CON_HANDLE_INVALID) {
            // disconnected
            task->status = PBIO_ERROR_NO_DEV;
            PT_EXIT(pt);
        }
        event_packet &&
        hci_event_packet_get_type(event_packet) == GATT_EVENT_QUERY_COMPLETE &&
        gatt_event_query_complete_get_handle(event_packet) == handset.con_handle;
    }));

    uint8_t status = gatt_event_query_complete_get_att_status(event_packet);
    task->status = att_error_to_pbio_error(status);

    PT_END(pt);
}

void pbdrv_bluetooth_write_remote(pbio_task_t *task, pbdrv_bluetooth_value_t *value) {
    pbio_task_init(task, write_remote_task, value);
    pbio_task_queue_add(task_queue, task);
}

void pbdrv_bluetooth_disconnect_remote(void) {
    if (handset.con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(handset.con_handle);
    }
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK
