// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

// Bluetooth driver using BlueKitchen BTStack.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK

#include <assert.h>
#include <inttypes.h>

#include <ble/gatt-service/device_information_service_server.h>
#include <ble/gatt-service/nordic_spp_service_server.h>
#include <btstack.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>

#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/version.h>

#include "bluetooth.h"
#include "bluetooth_btstack.h"

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
#include "genhdr/pybricks_service.h"
#include "pybricks_service_server.h"
#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

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

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
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

// hub name goes in special section so that it can be modified when flashing firmware
#if !PBIO_TEST_BUILD
__attribute__((section(".name")))
#endif
char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

static uint8_t *event_packet;
static const pbdrv_bluetooth_btstack_platform_data_t *pdata = &pbdrv_bluetooth_btstack_platform_data;

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
static hci_con_handle_t le_con_handle = HCI_CON_HANDLE_INVALID;
static hci_con_handle_t pybricks_con_handle = HCI_CON_HANDLE_INVALID;
static hci_con_handle_t uart_con_handle = HCI_CON_HANDLE_INVALID;
static pup_handset_t handset;
#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
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
    #endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

    return false;
}

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

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

static pbio_pybricks_error_t pybricks_data_received(hci_con_handle_t tx_con_handle, const uint8_t *data, uint16_t size) {
    if (pbdrv_bluetooth_receive_handler) {
        return pbdrv_bluetooth_receive_handler(data, size);
    }

    return ATT_ERROR_UNLIKELY_ERROR;
}

static void pybricks_configured(hci_con_handle_t tx_con_handle, uint16_t value) {
    pybricks_con_handle = value ? tx_con_handle : HCI_CON_HANDLE_INVALID;
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

static pbio_os_state_t bluetooth_thread_state;
static pbio_os_state_t bluetooth_thread_err;

/**
 * Runs tasks that may be waiting for event.
 *
 * This drives the common Bluetooth process synchronously with incoming events
 * so that each of the protothreads can wait for states.
 *
 * @param [in]  packet  Pointer to the raw packet data.
 */
static void propagate_event(uint8_t *packet) {

    event_packet = packet;

    if (bluetooth_thread_err == PBIO_ERROR_AGAIN) {
        bluetooth_thread_err = pbdrv_bluetooth_process_thread(&bluetooth_thread_state, NULL);
    }

    event_packet = NULL;
}

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
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
            // Not implemented.
            break;
        default:
            break;
    }

    propagate_event(packet);
}
#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

// currently, this function just handles the Powered Up handset control.
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    #endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_COMMAND_COMPLETE: {
            const uint8_t *rp = hci_event_command_complete_get_return_parameters(packet);
            switch (hci_event_command_complete_get_command_opcode(packet)) {
                case HCI_OPCODE_HCI_READ_LOCAL_VERSION_INFORMATION: {
                    uint16_t lmp_pal_subversion = pbio_get_uint16_le(&rp[7]);
                    pbdrv_bluetooth_btstack_set_chipset(lmp_pal_subversion);

                    #if DEBUG
                    // Show version in ev3dev format.
                    uint16_t chip = (lmp_pal_subversion & 0x7C00) >> 10;
                    uint16_t min_ver = (lmp_pal_subversion & 0x007F);
                    uint16_t maj_ver = (lmp_pal_subversion & 0x0380) >> 7;
                    if (lmp_pal_subversion & 0x8000) {
                        maj_ver |= 0x0008;
                    }
                    DEBUG_PRINT("LMP %04x: TIInit_%d.%d.%d.bts\n", lmp_pal_subversion, chip, maj_ver, min_ver);
                    #endif
                    break;
                }
                default:
                    break;
            }
            break;
        }
        #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
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
            if (peri->config->notification_handler) {
                uint16_t length = gatt_event_notification_get_value_length(packet);
                const uint8_t *value = gatt_event_notification_get_value(packet);
                peri->config->notification_handler(value, length);
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
                pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;
            } else {
                // If we aren't waiting for a peripheral connection, this must be a different connection.
                if (handset.con_state != CON_STATE_WAIT_CONNECT) {
                    break;
                }

                peri->con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                // Request pairing if needed for this device, otherwise set
                // connection state to complete.
                if (peri->config->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR) {
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

            if (pbdrv_bluetooth_observe_callback) {
                int8_t rssi = gap_event_advertising_report_get_rssi(packet);
                pbdrv_bluetooth_observe_callback(event_type, data, data_length, rssi);
            }

            if (handset.con_state == CON_STATE_WAIT_ADV_IND) {
                // Match advertisement data against context-specific filter.
                pbdrv_bluetooth_ad_match_result_flags_t adv_flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;
                if (peri->config->match_adv) {
                    adv_flags = peri->config->match_adv(event_type, data, NULL, address, peri->bdaddr);
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
                if (peri->config->match_adv_rsp) {
                    rsp_flags = peri->config->match_adv_rsp(event_type, NULL, detected_name, address, peri->bdaddr);
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
        #endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

        default:
            break;
    }

    propagate_event(packet);
}

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
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

pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    init_advertising_data();
    gap_advertisements_enable(true);

    PBIO_OS_AWAIT_UNTIL(state, event_packet && HCI_EVENT_IS_COMMAND_COMPLETE(event_packet, hci_le_set_advertise_enable));

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_ADVERTISING_PYBRICKS;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    gap_advertisements_enable(false);

    // REVISIT: use callback to await operation

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

typedef struct {
    const uint8_t *data;
    uint16_t size;
    bool done;
} send_data_t;

static void pybricks_on_ready_to_send(void *context) {
    send_data_t *send = context;
    pybricks_service_server_send(pybricks_con_handle, send->data, send->size);
    send->done = true;
    pbio_os_request_poll();
}

pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size) {

    static send_data_t send_data;

    static btstack_context_callback_registration_t send_request = {
        .callback = pybricks_on_ready_to_send,
        .context = &send_data,
    };
    PBIO_OS_ASYNC_BEGIN(state);

    send_data.data = data;
    send_data.size = size;
    send_data.done = false;
    pybricks_service_server_request_can_send_now(&send_request, pybricks_con_handle);
    PBIO_OS_AWAIT_UNTIL(state, send_data.done);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static void start_observing(void) {
    gap_set_scan_params(0, 0x30, 0x30, 0);
    gap_start_scan();
}

pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    // Scan and connect timeout, if applicable.
    bool timed_out = peri->config->timeout && pbio_os_timer_is_expired(&peri->timer);

    // Operation can be explicitly cancelled or automatically on inactivity.
    if (!peri->cancel) {
        peri->cancel = pbio_os_timer_is_expired(&peri->watchdog);
    }

    PBIO_OS_ASYNC_BEGIN(state);

    memset(&handset, 0, sizeof(handset));

    peri->con_handle = HCI_CON_HANDLE_INVALID;

    // active scanning to get scan response data.
    // scan interval: 48 * 0.625ms = 30ms
    gap_set_scan_params(1, 0x30, 0x30, 0);
    gap_start_scan();
    handset.con_state = CON_STATE_WAIT_ADV_IND;

    PBIO_OS_AWAIT_UNTIL(state, ({
        if (peri->cancel || timed_out) {
            if (handset.con_state == CON_STATE_WAIT_ADV_IND || handset.con_state == CON_STATE_WAIT_SCAN_RSP) {
                gap_stop_scan();
            } else if (handset.con_state == CON_STATE_WAIT_CONNECT) {
                gap_connect_cancel();
            } else if (peri->con_handle != HCI_CON_HANDLE_INVALID) {
                gap_disconnect(peri->con_handle);
            }
            handset.con_state = CON_STATE_NONE;
            return timed_out ? PBIO_ERROR_TIMEDOUT : PBIO_ERROR_CANCELED;
        }
        // if there is any failure to connect or error while enumerating
        // attributes, con_state will be set to CON_STATE_NONE
        if (handset.con_state == CON_STATE_NONE) {
            return PBIO_ERROR_FAILED;
        }
        handset.con_state == CON_STATE_CONNECTED;
    }));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PBIO_OS_ASYNC_BEGIN(state);

    if (handset.con_state != CON_STATE_CONNECTED) {
        return PBIO_ERROR_FAILED;
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

    PBIO_OS_AWAIT_UNTIL(state, ({
        // if there is any error while enumerating
        // attributes, con_state will be set to CON_STATE_NONE
        if (handset.con_state == CON_STATE_NONE) {
            return PBIO_ERROR_FAILED;
        }
        handset.con_state == CON_STATE_DISCOVERY_AND_NOTIFICATIONS_COMPLETE;
    }));

    // State state back to simply connected, so we can discover other characteristics.
    handset.con_state = CON_STATE_CONNECTED;

    PBIO_OS_ASYNC_END(peri->char_now->handle ? PBIO_SUCCESS : PBIO_ERROR_FAILED);
}

pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PBIO_OS_ASYNC_BEGIN(state);

    if (handset.con_state != CON_STATE_CONNECTED) {
        return PBIO_ERROR_FAILED;
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

    PBIO_OS_AWAIT_UNTIL(state, ({
        // if there is any error while reading, con_state will be set to CON_STATE_NONE
        if (handset.con_state == CON_STATE_NONE) {
            return PBIO_ERROR_FAILED;
        }
        handset.con_state == CON_STATE_READ_CHARACTERISTIC_COMPLETE;
    }));

    // State state back to simply connected, so we can discover other characteristics.
    handset.con_state = CON_STATE_CONNECTED;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PBIO_OS_ASYNC_BEGIN(state);

    uint8_t err = gatt_client_write_value_of_characteristic(packet_handler,
        peri->con_handle,
        pbdrv_bluetooth_char_write_handle,
        pbdrv_bluetooth_char_write_size,
        pbdrv_bluetooth_char_write_data
        );

    if (err != ERROR_CODE_SUCCESS) {
        return PBIO_ERROR_FAILED;
    }

    // NB: Value buffer must remain valid until GATT_EVENT_QUERY_COMPLETE, so
    // this wait is not cancelable.
    PBIO_OS_AWAIT_UNTIL(state, ({
        if (peri->con_handle == HCI_CON_HANDLE_INVALID) {
            // disconnected
            return PBIO_ERROR_NO_DEV;
        }
        event_packet &&
        hci_event_packet_get_type(event_packet) == GATT_EVENT_QUERY_COMPLETE &&
        gatt_event_query_complete_get_handle(event_packet) == peri->con_handle;
    }));

    PBIO_OS_ASYNC_END(att_error_to_pbio_error(gatt_event_query_complete_get_att_status(event_packet)));
}

pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    if (peri->con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(peri->con_handle);
    }

    PBIO_OS_AWAIT_UNTIL(state, peri->con_handle == HCI_CON_HANDLE_INVALID);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    gap_advertisements_set_data(pbdrv_bluetooth_broadcast_data_size, pbdrv_bluetooth_broadcast_data);

    // If already broadcasting, await set data and return.
    if (pbdrv_bluetooth_advertising_state == PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING) {
        PBIO_OS_AWAIT_UNTIL(state, event_packet && HCI_EVENT_IS_COMMAND_COMPLETE(event_packet, hci_le_set_advertising_data));
        return PBIO_SUCCESS;
    }

    bd_addr_t null_addr = { };
    gap_advertisements_set_params(0xA0, 0xA0, PBDRV_BLUETOOTH_AD_TYPE_ADV_NONCONN_IND, 0, null_addr, 0x7, 0);
    gap_advertisements_enable(true);

    PBIO_OS_AWAIT_UNTIL(state, event_packet && HCI_EVENT_IS_COMMAND_COMPLETE(event_packet, hci_le_set_advertise_enable));

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (!pbdrv_bluetooth_is_observing) {
        start_observing();
        pbdrv_bluetooth_is_observing = true;
        // REVISIT: use callback to await operation
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (pbdrv_bluetooth_is_observing) {
        gap_stop_scan();
        pbdrv_bluetooth_is_observing = false;
        // REVISIT: use callback to await operation
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}


#else // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

const char *pbdrv_bluetooth_get_hub_name(void) {
    return pbdrv_bluetooth_hub_name;
}

const char *pbdrv_bluetooth_get_fw_version(void) {
    // REVISIT: this should be linked to the init script as it can be updated in software
    // init script version
    return "v1.4";
}

void pbdrv_bluetooth_controller_reset_hard(void) {
    hci_power_control(HCI_POWER_OFF);
}

/**
 * btstack's hci_power_control() synchronously emits an event that would cause
 * it to re-enter the event loop. This would not be safe to call from within
 * the event loop. This wrapper ensures it is called at most once.
 */
static pbio_error_t bluetooth_btstack_handle_power_control(pbio_os_state_t *state, HCI_POWER_MODE power_mode, HCI_STATE end_state) {

    static bool busy_handling_power_control;

    PBIO_OS_ASYNC_BEGIN(state);

    if (busy_handling_power_control) {
        return PBIO_ERROR_AGAIN;
    }

    busy_handling_power_control = true;
    hci_power_control(power_mode); // causes synchronous re-entry.
    busy_handling_power_control = false;

    // Wait for the power state to take effect.
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == end_state);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // Disconnect gracefully if connected to host.
    if (le_con_handle != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(le_con_handle);
        PBIO_OS_AWAIT_UNTIL(state, le_con_handle == HCI_CON_HANDLE_INVALID);
    }

    // Wait for power off.
    PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_handle_power_control(&sub, HCI_POWER_OFF, HCI_STATE_OFF));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);

    #else
    // Revisit: CC2560X seems to dislike the reset command, but this is not
    // LE specific. Find a way to properly reset.
    return PBIO_ERROR_NOT_IMPLEMENTED;
    #endif
}

pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // Wait for power on.
    PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_handle_power_control(&sub, HCI_POWER_ON, HCI_STATE_WORKING));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static void bluetooth_btstack_run_loop_init(void) {
    // Not used. Bluetooth process is started like a regular pbdrv process.
}

static btstack_linked_list_t data_sources;

static void bluetooth_btstack_run_loop_add_data_source(btstack_data_source_t *ds) {
    btstack_linked_list_add(&data_sources, &ds->item);
}

static bool bluetooth_btstack_run_loop_remove_data_source(btstack_data_source_t *ds) {
    return btstack_linked_list_remove(&data_sources, &ds->item);
}

static void bluetooth_btstack_run_loop_enable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callback_types) {
    ds->flags |= callback_types;
}

static void bluetooth_btstack_run_loop_disable_data_source_callbacks(btstack_data_source_t *ds, uint16_t callback_types) {
    ds->flags &= ~callback_types;
}

static btstack_linked_list_t timers;

static void bluetooth_btstack_run_loop_set_timer(btstack_timer_source_t *ts, uint32_t timeout_in_ms) {
    ts->timeout = pbdrv_clock_get_ms() + timeout_in_ms;
}

static void bluetooth_btstack_run_loop_add_timer(btstack_timer_source_t *ts) {
    btstack_linked_item_t *it;
    for (it = (void *)&timers; it->next; it = it->next) {
        // don't add timer that's already in there
        btstack_timer_source_t *next = (void *)it->next;
        if (next == ts) {
            // timer was already in the list!
            assert(0);
            return;
        }
        // exit if new timeout before list timeout
        int32_t delta = btstack_time_delta(ts->timeout, next->timeout);
        if (delta < 0) {
            break;
        }
    }

    ts->item.next = it->next;
    it->next = &ts->item;
}

static bool bluetooth_btstack_run_loop_remove_timer(btstack_timer_source_t *ts) {
    if (btstack_linked_list_remove(&timers, &ts->item)) {
        return true;
    }
    return false;
}

static void bluetooth_btstack_run_loop_execute(void) {
    // not used
}

static void bluetooth_btstack_run_loop_dump_timer(void) {
    // not used
}

static const btstack_run_loop_t bluetooth_btstack_run_loop = {
    .init = bluetooth_btstack_run_loop_init,
    .add_data_source = bluetooth_btstack_run_loop_add_data_source,
    .remove_data_source = bluetooth_btstack_run_loop_remove_data_source,
    .enable_data_source_callbacks = bluetooth_btstack_run_loop_enable_data_source_callbacks,
    .disable_data_source_callbacks = bluetooth_btstack_run_loop_disable_data_source_callbacks,
    .set_timer = bluetooth_btstack_run_loop_set_timer,
    .add_timer = bluetooth_btstack_run_loop_add_timer,
    .remove_timer = bluetooth_btstack_run_loop_remove_timer,
    .execute = bluetooth_btstack_run_loop_execute,
    .dump_timer = bluetooth_btstack_run_loop_dump_timer,
    .get_time_ms = pbdrv_clock_get_ms,
};

static bool do_poll_handler;

void pbdrv_bluetooth_btstack_run_loop_trigger(void) {
    do_poll_handler = true;
    pbio_os_request_poll();
}

static pbio_os_process_t pbdrv_bluetooth_hci_process;

/**
 * This process is slightly unusual in that it does not have a state. It is
 * essentially just a poll handler.
 */
static pbio_error_t pbdrv_bluetooth_hci_process_thread(pbio_os_state_t *state, void *context) {

    if (do_poll_handler) {
        do_poll_handler = false;

        btstack_data_source_t *ds, *next;
        for (ds = (void *)data_sources; ds != NULL; ds = next) {
            // cache pointer to next data_source to allow data source to remove itself
            next = (void *)ds->item.next;
            if (ds->flags & DATA_SOURCE_CALLBACK_POLL) {
                ds->process(ds, DATA_SOURCE_CALLBACK_POLL);
            }
        }
    }

    static pbio_os_timer_t btstack_timer = {
        .duration = 1,
    };

    if (pbio_os_timer_is_expired(&btstack_timer)) {
        pbio_os_timer_extend(&btstack_timer);

        // process all BTStack timers in list that have expired
        while (timers) {
            btstack_timer_source_t *ts = (void *)timers;
            int32_t delta = btstack_time_delta(ts->timeout, pbdrv_clock_get_ms());
            if (delta > 0) {
                // we have reached unexpired timers
                break;
            }
            btstack_run_loop_remove_timer(ts);
            ts->process(ts);
        }
    }

    // Also propagate non-btstack events like polls or timers.
    propagate_event(NULL);

    return bluetooth_thread_err;
}

// Have to be defined, but are not used.
const uint32_t cc256x_init_script_size = 0;
const uint8_t cc256x_init_script[] = {};

void pbdrv_bluetooth_init_hci(void) {

    static btstack_packet_callback_registration_t hci_event_callback_registration;

    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
    // don't need to init the whole struct, so doing this here
    peripheral_singleton.con_handle = HCI_CON_HANDLE_INVALID;
    #endif

    btstack_memory_init();
    btstack_run_loop_init(&bluetooth_btstack_run_loop);

    hci_init(pdata->transport_instance(), pdata->transport_config());
    hci_set_chipset(pdata->chipset_instance());
    hci_set_control(pdata->control_instance());

    // REVISIT: do we need to call btstack_chipset_cc256x_set_power() or btstack_chipset_cc256x_set_power_vector()?

    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    l2cap_init();

    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE
    // setup LE device DB
    le_device_db_init();

    // setup security manager
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
    sm_set_er((uint8_t *)pdata->er_key);
    sm_set_ir((uint8_t *)pdata->ir_key);
    static btstack_packet_callback_registration_t sm_event_callback_registration;
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
    #endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

    bluetooth_thread_err = PBIO_ERROR_AGAIN;
    bluetooth_thread_state = 0;
    pbio_os_process_start(&pbdrv_bluetooth_hci_process, pbdrv_bluetooth_hci_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK
