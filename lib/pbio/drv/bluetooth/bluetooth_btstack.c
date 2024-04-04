// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

// Bluetooth driver using BlueKitchen BTStack.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK

#include <inttypes.h>

#include <ble/gatt-service/device_information_service_server.h>
#include <ble/gatt-service/nordic_spp_service_server.h>
#include <btstack.h>
#include <contiki.h>
#include <contiki-lib.h>

#include <pbdrv/bluetooth.h>
#include <pbio/protocol.h>
#include <pbio/task.h>
#include <pbio/version.h>

#include "bluetooth_btstack_run_loop_contiki.h"
#include "bluetooth_btstack.h"
#include "genhdr/pybricks_service.h"
#include "hci_transport_h4.h"
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

#if 0
#include <pbdrv/../../drv/ioport/ioport_debug_uart.h>
#define DEBUG_PRINT pbdrv_ioport_debug_uart_printf
#else
#define DEBUG_PRINT(...)
#endif

typedef enum {
    CON_STATE_NONE,
    CON_STATE_WAIT_ADV_IND,
    CON_STATE_WAIT_SCAN_RSP,
    CON_STATE_WAIT_CONNECT,
    CON_STATE_WAIT_BONDING,
    CON_STATE_CONNECTED, // End of connection state machine
    CON_STATE_WAIT_DISCOVER_CHARACTERISTICS,
    CON_STATE_WAIT_ENABLE_NOTIFICATIONS,
    CON_STATE_DISCOVERY_AND_NOTIFICATIONS_COMPLETE, // End of discovery state machine, goes back to CONNECTED
    CON_STATE_WAIT_READ_CHARACTERISTIC,
    CON_STATE_READ_CHARACTERISTIC_COMPLETE, // End of read state machine, goes back to CONNECTED
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

// REVISIT: Most of these states should go into pbdrv_bluetooth_peripheral_t
typedef struct {
    gatt_client_notification_t notification;
    con_state_t con_state;
    disconnect_reason_t disconnect_reason;
    /**
     *  Character information used during discovery. Assuming properties and chars
     *  are set up such that only one char is discovered at a time
     */
    gatt_client_characteristic_t current_char;
    uint8_t btstack_error;
} pup_handset_t;

// The peripheral singleton. Used to connect to a device like the LEGO Remote.
pbdrv_bluetooth_peripheral_t peripheral_singleton;

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
static pup_handset_t handset;
static uint8_t *event_packet;
static const pbdrv_bluetooth_btstack_platform_data_t *pdata = &pbdrv_bluetooth_btstack_platform_data;

static bool is_broadcasting;
static bool is_observing;
static pbdrv_bluetooth_start_observing_callback_t observe_callback;

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

/**
 * Queues a tasks and runs the first iteration if there is no other task already
 * running.
 *
 * @param [in]  task    An uninitialized task.
 * @param [in]  thread  The task thread to attach to the task.
 * @param [in]  context The context to attach to the task.
 */
static void start_task(pbio_task_t *task, pbio_task_thread_t thread, void *context) {
    pbio_task_init(task, thread, context);

    if (list_head(task_queue) != NULL || !pbio_task_run_once(task)) {
        list_add(task_queue, task);
    }
}

/**
 * Runs tasks that may be waiting for event and notifies external subscriber.
 *
 * @param [in]  packet  Pointer to the raw packet data.
 */
static void propagate_event(uint8_t *packet) {
    event_packet = packet;

    for (;;) {
        pbio_task_t *current_task = list_head(task_queue);

        if (!current_task) {
            break;
        }

        if (current_task->status != PBIO_ERROR_AGAIN || pbio_task_run_once(current_task)) {
            // remove the task from the queue only if the task is complete
            list_remove(task_queue, current_task);
            // then start the next task
            continue;
        }

        break;
    }

    event_packet = NULL;

    if (bluetooth_on_event) {
        bluetooth_on_event();
    }
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

    propagate_event(packet);
}

// currently, this function just handles the Powered Up handset control.
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_SERVICE_QUERY_RESULT: {
            // Service discovery not used.
            gatt_client_service_t service;
            gatt_event_service_query_result_get_service(packet, &service);
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
            gatt_client_characteristic_t found_char;
            gatt_event_characteristic_query_result_get_characteristic(packet, &found_char);
            // We only care about the one characteristic that has at least the requested properties.
            if ((found_char.properties & peri->char_now->properties) == peri->char_now->properties) {
                peri->char_now->handle = found_char.value_handle;
                gatt_event_characteristic_query_result_get_characteristic(packet, &handset.current_char);
            }
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT: {
            hci_con_handle_t handle = gatt_event_characteristic_value_query_result_get_handle(packet);
            uint16_t value_handle = gatt_event_characteristic_value_query_result_get_value_handle(packet);
            uint16_t value_length = gatt_event_characteristic_value_query_result_get_value_length(packet);
            if (peri->con_handle == handle && peri->char_now->handle == value_handle) {
                peri->char_now->value_len = gatt_event_characteristic_value_query_result_get_value_length(packet);
                memcpy(peri->char_now->value, gatt_event_characteristic_value_query_result_get_value(packet), value_length);
            }
            break;
        }
        case GATT_EVENT_QUERY_COMPLETE:
            if (handset.con_state == CON_STATE_WAIT_READ_CHARACTERISTIC) {
                // Done reading characteristic.
                handset.con_state = CON_STATE_READ_CHARACTERISTIC_COMPLETE;
            } else if (handset.con_state == CON_STATE_WAIT_DISCOVER_CHARACTERISTICS) {

                // Discovered characteristics, ready enable notifications.
                if (!peri->char_now->request_notification) {
                    // If no notification is requested, we are done.
                    handset.con_state = CON_STATE_DISCOVERY_AND_NOTIFICATIONS_COMPLETE;
                    break;
                }

                handset.btstack_error = gatt_client_write_client_characteristic_configuration(
                    packet_handler, peri->con_handle, &handset.current_char,
                    GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
                if (handset.btstack_error == ERROR_CODE_SUCCESS) {
                    gatt_client_listen_for_characteristic_value_updates(
                        &handset.notification, packet_handler, peri->con_handle, &handset.current_char);
                    handset.con_state = CON_STATE_WAIT_ENABLE_NOTIFICATIONS;
                } else {
                    // configuration failed for some reason, so disconnect
                    gap_disconnect(peri->con_handle);
                    handset.con_state = CON_STATE_WAIT_DISCONNECT;
                    handset.disconnect_reason = DISCONNECT_REASON_CONFIGURE_CHARACTERISTIC_FAILED;
                }
            } else if (handset.con_state == CON_STATE_WAIT_ENABLE_NOTIFICATIONS) {
                // Done enabling notifications.
                handset.con_state = CON_STATE_DISCOVERY_AND_NOTIFICATIONS_COMPLETE;
            }
            break;

        case GATT_EVENT_NOTIFICATION: {
            if (gatt_event_notification_get_handle(packet) != peri->con_handle) {
                break;
            }
            if (peri->notification_handler) {
                uint16_t length = gatt_event_notification_get_value_length(packet);
                const uint8_t *value = gatt_event_notification_get_value(packet);
                peri->notification_handler(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL, value, length);
            }
            break;
        }

        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                break;
            }

            // HCI_ROLE_SLAVE means the connecting device is the central and the hub is the peripheral
            // HCI_ROLE_MASTER means the connecting device is the peripheral and the hub is the central.
            if (hci_subevent_le_connection_complete_get_role(packet) == HCI_ROLE_SLAVE) {
                le_con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                // don't start advertising again on disconnect
                gap_advertisements_enable(false);
            } else {
                // If we aren't waiting for a peripheral connection, this must be a different connection.
                if (handset.con_state != CON_STATE_WAIT_CONNECT) {
                    break;
                }

                peri->con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                // Request pairing if needed for this device, otherwise set
                // connection state to complete.
                if (peri->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR) {
                    // Re-encryption doesn't seem to work reliably, so we just
                    // delete the bond and start over.
                    gap_delete_bonding(peri->bdaddr_type, peri->bdaddr);
                    sm_request_pairing(peri->con_handle);
                    handset.con_state = CON_STATE_WAIT_BONDING;
                } else {
                    handset.con_state = CON_STATE_CONNECTED;
                }
            }

            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            if (hci_event_disconnection_complete_get_connection_handle(packet) == le_con_handle) {
                le_con_handle = HCI_CON_HANDLE_INVALID;
                pybricks_con_handle = HCI_CON_HANDLE_INVALID;
                uart_con_handle = HCI_CON_HANDLE_INVALID;
            } else if (hci_event_disconnection_complete_get_connection_handle(packet) == peri->con_handle) {
                gatt_client_stop_listening_for_characteristic_value_updates(&handset.notification);
                peri->con_handle = HCI_CON_HANDLE_INVALID;
                handset.con_state = CON_STATE_NONE;
            }

            break;

        case GAP_EVENT_ADVERTISING_REPORT: {
            uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
            uint8_t data_length = gap_event_advertising_report_get_data_length(packet);
            const uint8_t *data = gap_event_advertising_report_get_data(packet);
            bd_addr_t address;

            gap_event_advertising_report_get_address(packet, address);

            if (observe_callback) {
                int8_t rssi = gap_event_advertising_report_get_rssi(packet);
                observe_callback(event_type, data, data_length, rssi);
            }

            if (handset.con_state == CON_STATE_WAIT_ADV_IND) {
                // Match advertisement data against context-specific filter.
                pbdrv_bluetooth_ad_match_result_flags_t adv_flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;
                if (peri->match_adv) {
                    adv_flags = peri->match_adv(event_type, data, NULL, address, peri->bdaddr);
                }

                if (adv_flags & PBDRV_BLUETOOTH_AD_MATCH_VALUE) {
                    if (adv_flags & PBDRV_BLUETOOTH_AD_MATCH_ADDRESS) {
                        // This was the same device as last time. If the scan response
                        // didn't match before, it probably won't match now and we
                        // should try a different device.
                        break;
                    }
                    // Advertising data matched, prepare for scan response.
                    memcpy(peri->bdaddr, address, sizeof(bd_addr_t));
                    peri->bdaddr_type = gap_event_advertising_report_get_address_type(packet);
                    handset.con_state = CON_STATE_WAIT_SCAN_RSP;
                }
            } else if (handset.con_state == CON_STATE_WAIT_SCAN_RSP) {

                char *detected_name = (char *)&data[2];
                const uint8_t max_len = sizeof(peri->name);

                pbdrv_bluetooth_ad_match_result_flags_t rsp_flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;
                if (peri->match_adv_rsp) {
                    rsp_flags = peri->match_adv_rsp(event_type, NULL, detected_name, address, peri->bdaddr);
                }
                if ((rsp_flags & PBDRV_BLUETOOTH_AD_MATCH_VALUE) && (rsp_flags & PBDRV_BLUETOOTH_AD_MATCH_ADDRESS)) {

                    if (rsp_flags & PBDRV_BLUETOOTH_AD_MATCH_NAME_FAILED) {
                        // A name was requested but it doesn't match, so go back to scanning stage.
                        handset.con_state = CON_STATE_WAIT_ADV_IND;
                        break;
                    }

                    if (data[1] == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) {
                        memcpy(peri->name, detected_name, max_len);
                    }

                    gap_stop_scan();
                    handset.btstack_error = gap_connect(peri->bdaddr, peri->bdaddr_type);

                    if (handset.btstack_error == ERROR_CODE_SUCCESS) {
                        handset.con_state = CON_STATE_WAIT_CONNECT;
                    } else {
                        handset.con_state = CON_STATE_NONE;
                    }
                }
            }

            break;
        }

        default:
            break;
    }

    propagate_event(packet);
}

// Security manager callbacks. This is adapted from the BTstack examples.
static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    bd_addr_t addr;
    bd_addr_type_t addr_type;

    switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_IDENTITY_RESOLVING_STARTED:
            DEBUG_PRINT("IDENTITY_RESOLVING_STARTED\n");
            break;
        case SM_EVENT_IDENTITY_RESOLVING_FAILED:
            DEBUG_PRINT("IDENTITY_RESOLVING_FAILED\n");
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            // This is the only expected path for known compatible peripherals.
            DEBUG_PRINT("Just works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            DEBUG_PRINT("Confirming numeric comparison: %" PRIu32 "\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            DEBUG_PRINT("Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case SM_EVENT_PASSKEY_INPUT_NUMBER: {
            const uint32_t passkey = 123456U;
            DEBUG_PRINT("Passkey Input requested\n");
            DEBUG_PRINT("Sending fixed passkey %" PRIu32 "\n", passkey);
            sm_passkey_input(sm_event_passkey_input_number_get_handle(packet), passkey);
            break;
        }
        case SM_EVENT_PAIRING_STARTED:
            DEBUG_PRINT("Pairing started\n");
            break;
        case SM_EVENT_PAIRING_COMPLETE:
            switch (sm_event_pairing_complete_get_status(packet)) {
                case ERROR_CODE_SUCCESS:
                    // This is the final state for known compatible peripherals
                    // with bonding under normal circumstances.
                    DEBUG_PRINT("Pairing complete, success\n");
                    handset.con_state = CON_STATE_CONNECTED;
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                // fall through to disconnect.
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                // fall through to disconnect.
                case ERROR_CODE_AUTHENTICATION_FAILURE:
                // fall through to disconnect.
                default:
                    DEBUG_PRINT(
                        "Pairing completed with error code %u and reason %u\n",
                        sm_event_pairing_complete_get_status(packet),
                        sm_event_pairing_complete_get_reason(packet)
                        );
                    gap_disconnect(sm_event_reencryption_complete_get_handle(packet));
                    break;
            }
            break;
        case SM_EVENT_REENCRYPTION_STARTED:
            sm_event_reencryption_complete_get_address(packet, addr);
            DEBUG_PRINT("Bonding information exists for addr type %u, identity addr %s -> start re-encryption\n",
                sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        case SM_EVENT_REENCRYPTION_COMPLETE:
            // BTstack supports re-encryption, but it gets the hub in a hung
            // state with certain peripherals. Instead we just delete the bond
            // just before connecting. If we still get here, we should delete
            // the bond and disconnect, causing the user program stop without
            // hanging. We rely on HCI_EVENT_DISCONNECTION_COMPLETE to set
            // the connection state appropriately to unblock the task.
            DEBUG_PRINT("Re-encryption complete. Handling not implemented.\n");
            sm_event_reencryption_complete_get_address(packet, addr);
            addr_type = sm_event_reencryption_started_get_addr_type(packet);
            gap_delete_bonding(addr_type, addr);
            gap_disconnect(sm_event_reencryption_complete_get_handle(packet));
            break;
        default:
            break;
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
    static btstack_packet_callback_registration_t sm_event_callback_registration;

    // don't need to init the whole struct, so doing this here
    peripheral_singleton.con_handle = HCI_CON_HANDLE_INVALID;

    btstack_memory_init();
    btstack_run_loop_init(pbdrv_bluetooth_btstack_run_loop_contiki_get_instance());

    hci_init(hci_transport_h4_instance_for_uart(pdata->uart_block_instance()), &config);
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
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
    sm_set_er((uint8_t *)pdata->er_key);
    sm_set_ir((uint8_t *)pdata->ir_key);
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    gap_random_address_set_mode(GAP_RANDOM_ADDRESS_NON_RESOLVABLE);
    gap_set_max_number_peripheral_connections(2);

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

    // When powering off, cancel all pending tasks.
    if (!on) {
        pbio_task_t *task;

        while ((task = list_pop(task_queue)) != NULL) {
            if (task->status == PBIO_ERROR_AGAIN) {
                task->status = PBIO_ERROR_CANCELED;
            }
        }
    }
}

bool pbdrv_bluetooth_is_ready(void) {
    return hci_get_state() != HCI_STATE_OFF;
}

const char *pbdrv_bluetooth_get_hub_name(void) {
    return pbdrv_bluetooth_hub_name;
}

const char *pbdrv_bluetooth_get_fw_version(void) {
    // REVISIT: this should be linked to the init script as it can be updated in software
    // init script version
    return "v1.4";
}

static PT_THREAD(noop_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);
    task->status = PBIO_SUCCESS;
    PT_END(pt);
}

void pbdrv_bluetooth_queue_noop(pbio_task_t *task) {
    start_task(task, noop_task, NULL);
}

static void init_advertising_data(void) {
    bd_addr_t null_addr = { };
    gap_advertisements_set_params(0x30, 0x30, PBDRV_BLUETOOTH_AD_TYPE_ADV_IND, 0x00, null_addr, 0x07, 0x00);

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

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL && peripheral_singleton.con_handle != HCI_CON_HANDLE_INVALID) {
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

static void start_observing(void) {
    gap_set_scan_params(0, 0x30, 0x30, 0);
    gap_start_scan();
}

static PT_THREAD(peripheral_scan_and_connect_task(struct pt *pt, pbio_task_t *task)) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PT_BEGIN(pt);

    memset(&handset, 0, sizeof(handset));

    peri->con_handle = HCI_CON_HANDLE_INVALID;

    // active scanning to get scan response data.
    // scan interval: 48 * 0.625ms = 30ms
    gap_set_scan_params(1, 0x30, 0x30, 0);
    gap_start_scan();
    handset.con_state = CON_STATE_WAIT_ADV_IND;

    PT_WAIT_UNTIL(pt, ({
        if (task->cancel) {
            goto cancel;
        }

        // if there is any failure to connect or error while enumerating
        // attributes, con_state will be set to CON_STATE_NONE
        if (handset.con_state == CON_STATE_NONE) {
            task->status = PBIO_ERROR_FAILED;
            PT_EXIT(pt);
        }

        handset.con_state == CON_STATE_CONNECTED;
    }));

    task->status = PBIO_SUCCESS;
    goto out;

cancel:
    if (handset.con_state == CON_STATE_WAIT_ADV_IND || handset.con_state == CON_STATE_WAIT_SCAN_RSP) {
        gap_stop_scan();
    } else if (handset.con_state == CON_STATE_WAIT_CONNECT) {
        gap_connect_cancel();
    } else if (peri->con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(peri->con_handle);
    }
    handset.con_state = CON_STATE_NONE;
    task->status = PBIO_ERROR_CANCELED;

out:
    // restore observing state
    if (is_observing) {
        start_observing();
    }

    PT_END(pt);
}

static PT_THREAD(periperal_discover_characteristic_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    PT_BEGIN(pt);

    if (handset.con_state != CON_STATE_CONNECTED) {
        task->status = PBIO_ERROR_FAILED;
        PT_EXIT(pt);
    }

    handset.con_state = CON_STATE_WAIT_DISCOVER_CHARACTERISTICS;
    uint16_t handle_max = peri->char_now->handle_max ? peri->char_now->handle_max : 0xffff;
    handset.btstack_error = peri->char_now->uuid16 ?
        gatt_client_discover_characteristics_for_handle_range_by_uuid16(
        packet_handler, peri->con_handle, 0x0001, handle_max, peri->char_now->uuid16) :
        gatt_client_discover_characteristics_for_handle_range_by_uuid128(
        packet_handler, peri->con_handle, 0x0001, handle_max, peri->char_now->uuid128);

    if (handset.btstack_error != ERROR_CODE_SUCCESS) {
        // configuration failed for some reason, so disconnect
        gap_disconnect(peri->con_handle);
        handset.con_state = CON_STATE_WAIT_DISCONNECT;
        handset.disconnect_reason = DISCONNECT_REASON_DISCOVER_CHARACTERISTIC_FAILED;
    }

    PT_WAIT_UNTIL(pt, ({
        if (task->cancel) {
            goto cancel;
        }

        // if there is any error while enumerating
        // attributes, con_state will be set to CON_STATE_NONE
        if (handset.con_state == CON_STATE_NONE) {
            task->status = PBIO_ERROR_FAILED;
            PT_EXIT(pt);
        }

        handset.con_state == CON_STATE_DISCOVERY_AND_NOTIFICATIONS_COMPLETE;
    }));

    // State state back to simply connected, so we can discover other characteristics.
    handset.con_state = CON_STATE_CONNECTED;

    task->status = peri->char_now->handle ? PBIO_SUCCESS : PBIO_ERROR_FAILED;
    PT_EXIT(pt);

cancel:
    if (peri->con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(peri->con_handle);
    }
    handset.con_state = CON_STATE_NONE;
    task->status = PBIO_ERROR_CANCELED;

    PT_END(pt);
}

void pbdrv_bluetooth_peripheral_scan_and_connect(pbio_task_t *task, pbdrv_bluetooth_ad_match_t match_adv, pbdrv_bluetooth_ad_match_t match_adv_rsp, pbdrv_bluetooth_receive_handler_t notification_handler, pbdrv_bluetooth_peripheral_options_t options) {
    // Unset previous bluetooth addresses and other state variables.
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    memset(peri, 0, sizeof(pbdrv_bluetooth_peripheral_t));

    // Set scan filters and notification handler, then start scannning.
    peri->match_adv = match_adv;
    peri->match_adv_rsp = match_adv_rsp;
    peri->notification_handler = notification_handler;
    peri->options = options;
    start_task(task, peripheral_scan_and_connect_task, NULL);
}

void pbdrv_bluetooth_periperal_discover_characteristic(pbio_task_t *task, pbdrv_bluetooth_peripheral_char_t *characteristic) {
    characteristic->handle = 0;
    peripheral_singleton.char_now = characteristic;
    start_task(task, periperal_discover_characteristic_task, NULL);
}

static PT_THREAD(periperal_read_characteristic_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    PT_BEGIN(pt);

    if (handset.con_state != CON_STATE_CONNECTED) {
        task->status = PBIO_ERROR_FAILED;
        PT_EXIT(pt);
    }

    gatt_client_characteristic_t characteristic = {
        .value_handle = peri->char_now->handle,
    };
    handset.btstack_error = gatt_client_read_value_of_characteristic(packet_handler, peri->con_handle, &characteristic);

    if (handset.btstack_error == ERROR_CODE_SUCCESS) {
        handset.con_state = CON_STATE_WAIT_READ_CHARACTERISTIC;
    } else {
        // configuration failed for some reason, so disconnect
        gap_disconnect(peri->con_handle);
        handset.con_state = CON_STATE_WAIT_DISCONNECT;
        handset.disconnect_reason = DISCONNECT_REASON_DISCOVER_CHARACTERISTIC_FAILED;
    }

    PT_WAIT_UNTIL(pt, ({
        if (task->cancel) {
            goto cancel;
        }

        // if there is any error while reading, con_state will be set to CON_STATE_NONE
        if (handset.con_state == CON_STATE_NONE) {
            task->status = PBIO_ERROR_FAILED;
            PT_EXIT(pt);
        }

        handset.con_state == CON_STATE_READ_CHARACTERISTIC_COMPLETE;
    }));

    // State state back to simply connected, so we can discover other characteristics.
    handset.con_state = CON_STATE_CONNECTED;

    task->status = PBIO_SUCCESS;
    PT_EXIT(pt);

cancel:
    if (peri->con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(peri->con_handle);
    }
    handset.con_state = CON_STATE_NONE;
    task->status = PBIO_ERROR_CANCELED;

    PT_END(pt);
}

void pbdrv_bluetooth_periperal_read_characteristic(pbio_task_t *task, pbdrv_bluetooth_peripheral_char_t *characteristic) {
    peripheral_singleton.char_now = characteristic;
    start_task(task, periperal_read_characteristic_task, NULL);
}

const char *pbdrv_bluetooth_peripheral_get_name(void) {
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    return peri->name;
}

static PT_THREAD(peripheral_write_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_value_t *value = task->context;

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PT_BEGIN(pt);

    uint8_t err = gatt_client_write_value_of_characteristic(packet_handler,
        peri->con_handle, pbio_get_uint16_le(value->handle), value->size, value->data);

    if (err != ERROR_CODE_SUCCESS) {
        task->status = PBIO_ERROR_FAILED;
        PT_EXIT(pt);
    }

    // NB: Value buffer must remain valid until GATT_EVENT_QUERY_COMPLETE, so
    // this wait is not cancelable.
    PT_WAIT_UNTIL(pt, ({
        if (peri->con_handle == HCI_CON_HANDLE_INVALID) {
            // disconnected
            task->status = PBIO_ERROR_NO_DEV;
            PT_EXIT(pt);
        }
        event_packet &&
        hci_event_packet_get_type(event_packet) == GATT_EVENT_QUERY_COMPLETE &&
        gatt_event_query_complete_get_handle(event_packet) == peri->con_handle;
    }));

    uint8_t status = gatt_event_query_complete_get_att_status(event_packet);
    task->status = att_error_to_pbio_error(status);

    PT_END(pt);
}

void pbdrv_bluetooth_peripheral_write(pbio_task_t *task, pbdrv_bluetooth_value_t *value) {
    start_task(task, peripheral_write_task, value);
}

static PT_THREAD(peripheral_disconnect_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    if (peri->con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(peri->con_handle);
    }

    PT_WAIT_UNTIL(pt, peri->con_handle == HCI_CON_HANDLE_INVALID);

    task->status = PBIO_SUCCESS;
    PT_END(pt);
}

void pbdrv_bluetooth_peripheral_disconnect(pbio_task_t *task) {
    start_task(task, peripheral_disconnect_task, NULL);
}

static PT_THREAD(start_broadcasting_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_value_t *value = task->context;

    PT_BEGIN(pt);

    if (value->size > LE_ADVERTISING_DATA_SIZE) {
        task->status = PBIO_ERROR_INVALID_ARG;
        PT_EXIT(pt);
    }

    // have to keep copy of data here since BTStack doesn't copy
    static uint8_t static_data[LE_ADVERTISING_DATA_SIZE];
    memcpy(static_data, value->data, value->size);

    gap_advertisements_set_data(value->size, static_data);

    if (!is_broadcasting) {
        bd_addr_t null_addr = { };
        gap_advertisements_set_params(0xA0, 0xA0, PBDRV_BLUETOOTH_AD_TYPE_ADV_NONCONN_IND, 0, null_addr, 0x7, 0);
        gap_advertisements_enable(true);
        is_broadcasting = true;
    }

    // Wait advertising enable command to complete.
    PT_WAIT_UNTIL(pt, event_packet && HCI_EVENT_IS_COMMAND_COMPLETE(event_packet, hci_le_set_advertising_data));

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_start_broadcasting(pbio_task_t *task, pbdrv_bluetooth_value_t *value) {
    start_task(task, start_broadcasting_task, value);
}

static PT_THREAD(stop_broadcasting_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    if (is_broadcasting) {
        gap_advertisements_enable(false);
        is_broadcasting = false;
    }

    // REVISIT: use callback to actually wait for stop?
    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_stop_broadcasting(pbio_task_t *task) {
    start_task(task, stop_broadcasting_task, NULL);
}

static PT_THREAD(start_observing_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_start_observing_callback_t callback = task->context;

    PT_BEGIN(pt);

    observe_callback = callback;

    if (!is_observing) {
        start_observing();
        is_observing = true;
    }

    // REVISIT: use callback to actually wait for start?
    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_start_observing(pbio_task_t *task, pbdrv_bluetooth_start_observing_callback_t callback) {
    start_task(task, start_observing_task, callback);
}

static PT_THREAD(stop_observing_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    if (is_observing) {
        gap_stop_scan();
        is_observing = false;
    }

    // REVISIT: use callback to actually wait for stop?
    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_stop_observing(pbio_task_t *task) {
    observe_callback = NULL;
    start_task(task, stop_observing_task, NULL);
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK
