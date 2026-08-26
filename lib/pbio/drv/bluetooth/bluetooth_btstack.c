// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

// Bluetooth driver using BlueKitchen BTStack.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include <ble/gatt-service/device_information_service_server.h>
#include <ble/gatt-service/nordic_spp_service_server.h>
#include <btstack.h>
#include <btstack_run_loop.h>

#include <pbio/bluetooth.h>
#include <pbdrv/clock.h>

#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbio/version.h>

#include <pbdrv/bluetooth.h>
#include "bluetooth_btstack.h"

#include "genhdr/pybricks_service.h"
#include "pybricks_service_server.h"

// Timeouts for various steps in the scan and connect process.
#define PERIPHERAL_TIMEOUT_MS_CONNECT       (5000)
#define PERIPHERAL_TIMEOUT_MS_PAIRING       (5000)

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

#if DEBUG == 2
static void btstack_hci_dump_reset(void) {
}
static void btstack_hci_dump_log_packet(uint8_t packet_type, uint8_t in, uint8_t *packet, uint16_t len) {
    pbio_debug("HCI %s packet type: %02x, len: %u\n", in ? "in" : "out", packet_type, len);
}
static void btstack_hci_dump_log_message(int log_level, const char *format, va_list argptr) {
    pbio_debug_va(format, argptr);
    pbio_debug("\n");
}
static const hci_dump_t pbdrv_bluetooth_btstack_hci_dump = {
    .reset = btstack_hci_dump_reset,
    .log_packet = btstack_hci_dump_log_packet,
    .log_message = btstack_hci_dump_log_message,
};
#endif

/**
 * BTstack state that we need to maintain for each peripheral.
 */
static struct _pbio_bluetooth_peripheral_platform_state_t {
    /**
     * Current connection state, needed to unset callback on disconnect.
     */
    gatt_client_notification_t notification;
    /**
     *  Character information used during discovery. Assuming properties and chars
     *  are set up such that only one char is discovered at a time
     */
    gatt_client_characteristic_t current_char;
    /** Advertisement matched during scan; address and data recorded in peri. */
    bool adv_matched;
    /** Scan response matched during scan; name and data recorded in peri. */
    bool scan_rsp_matched;
    /** SM pairing or re-encryption completed. */
    bool pairing_complete;
    /** Pairing completed via re-encryption rather than a new pairing. */
    bool pairing_via_reencryption;
    /** Status byte from the SM pairing or re-encryption complete event. */
    uint8_t pairing_status;
    /** Last GATT client operation completed. */
    bool gatt_query_complete;
    /** ATT status of the last completed GATT client operation. */
    uint8_t gatt_att_status;
} peripheral_platform_state[PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS];

static pbio_bluetooth_peripheral_t _peripherals[PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS];

pbio_bluetooth_peripheral_t *pbdrv_bluetooth_peripheral_get_by_index(uint8_t index) {
    if (index >= PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS) {
        return NULL;
    }
    return &_peripherals[index];
}

// hub name goes in special section so that it can be modified when flashing firmware
#if !PBIO_TEST_BUILD
__attribute__((section(".name")))
#endif
char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

static const pbdrv_bluetooth_btstack_platform_data_t *pdata = &pbdrv_bluetooth_btstack_platform_data;

/**
 * Event outcomes recorded by the packet handlers for the driver thread to
 * consume. The thread clears a flag before issuing the command that triggers
 * it, then awaits the flag.
 */
static struct {
    /** LE set advertise enable command completed (fires for disable too). */
    bool advertise_enable_complete;
    /** LE set advertising data command completed. */
    bool advertising_data_complete;
    /**
     * Peripheral currently scanning/connecting. Advertising reports and the
     * connection complete event are recorded here. NULL when not scanning.
     * Once it has a handle, we can look it up directly for normal operations.
     */
    pbio_bluetooth_peripheral_t *scanning_peri;
} recorded_events;

/**
 * State of a connected host (Pybricks Code or similar).
 */
typedef struct {
    /** Connection handle. */
    hci_con_handle_t con_handle;
    /** Pybricks service is configured. */
    bool pybricks_configured;
    /** UART service is configured. */
    bool uart_configured;
    /** Notification to send when ready. */
    btstack_context_callback_registration_t send_request;
    /** Notification to send. */
    const uint8_t *notification_data;
    /** Notification size to send. */
    uint16_t notification_size;
    /** Notification has been sent. */
    bool notification_done;
} pbdrv_bluetooth_btstack_host_connection_t;

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
static pbdrv_bluetooth_btstack_host_connection_t host_connections[PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS];
#endif

static pbdrv_bluetooth_btstack_host_connection_t *pbdrv_bluetooth_btstack_get_host_connection(hci_con_handle_t con_handle) {
    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    for (size_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS; i++) {
        pbdrv_bluetooth_btstack_host_connection_t *host = &host_connections[i];
        if (host->con_handle == con_handle) {
            return host;
        }
    }
    #endif
    return NULL;
}

static pbio_bluetooth_peripheral_t *pbdrv_bluetooth_btstack_get_peripheral(hci_con_handle_t con_handle) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
        pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_peripheral_get_by_index(i);
        if (peri && peri->con_handle == con_handle) {
            return peri;
        }
    }
    return NULL;
}

/**
 * Converts BTStack error to most appropriate PBIO error.
 * @param [in]  status      The BTStack error
 * @return                  The PBIO error.
 */
static pbio_error_t att_error_to_pbio_error(uint8_t status) {
    switch (status) {
        case ATT_ERROR_SUCCESS:
            return PBIO_SUCCESS;
        case ATT_ERROR_INSUFFICIENT_AUTHENTICATION:
            return PBIO_ERROR_INVALID_OP;
        case ATT_ERROR_HCI_DISCONNECT_RECEIVED:
            return PBIO_ERROR_NO_DEV;
        case ATT_ERROR_TIMEOUT:
            return PBIO_ERROR_TIMEDOUT;
        case BTSTACK_MEMORY_ALLOC_FAILED:
            DEBUG_PRINT("BTstack out of memory.\n");
        // fallthrough
        default:
            DEBUG_PRINT("Failed with status.%d\n", status);
            return PBIO_ERROR_FAILED;
    }
}

static pbio_pybricks_error_t pybricks_data_received(hci_con_handle_t tx_con_handle, const uint8_t *data, uint16_t size) {
    // Treating all incoming host data the same.
    return pbio_bluetooth_receive_handler(data, size);
}

static void pybricks_configured(hci_con_handle_t tx_con_handle, uint16_t value) {
    pbdrv_bluetooth_btstack_host_connection_t *host = pbdrv_bluetooth_btstack_get_host_connection(tx_con_handle);
    if (host == NULL) {
        return;
    }
    host->pybricks_configured = !!value;
    pbio_bluetooth_host_connection_changed();
}

/**
 * Wrapper for gap_disconnect that is safe to call if already disconnected.
 */
static void pbdrv_bluetooth_peripheral_disconnect_now(pbio_bluetooth_peripheral_t *peri) {
    if (peri->con_handle == HCI_CON_HANDLE_INVALID) {
        // Nothing to do. The synchronous callback from gap_disconnect() is no
        // longer a reentrancy hazard since packet handlers only record state,
        // but skipping avoids a needless error on an invalid handle.
        return;
    }
    gap_disconnect(peri->con_handle);
}


static pbio_os_state_t bluetooth_thread_state;
static pbio_error_t bluetooth_thread_err;

bool pbdrv_bluetooth_peripheral_is_connected(pbio_bluetooth_peripheral_t *peri) {
    return peri->con_handle != HCI_CON_HANDLE_INVALID;
}

bool pbdrv_bluetooth_host_is_connected(void) {
    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    for (size_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS; i++) {
        pbdrv_bluetooth_btstack_host_connection_t *host = &host_connections[i];
        if (host->con_handle != HCI_CON_HANDLE_INVALID) {
            return true;
        }
    }
    #endif
    return false;
}

bool pbdrv_bluetooth_hci_is_enabled(void) {
    return bluetooth_thread_err == PBIO_ERROR_AGAIN;
}

static void nordic_spp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            if (hci_event_packet_get_type(packet) != HCI_EVENT_GATTSERVICE_META) {
                break;
            }

            switch (hci_event_gattservice_meta_get_subevent_code(packet)) {
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_CONNECTED: {
                    uint16_t handle = gattservice_subevent_spp_service_connected_get_con_handle(packet);
                    pbdrv_bluetooth_btstack_host_connection_t *host = pbdrv_bluetooth_btstack_get_host_connection(handle);
                    if (host) {
                        host->uart_configured = true;
                    }
                    break;
                }
                case GATTSERVICE_SUBEVENT_SPP_SERVICE_DISCONNECTED: {
                    uint16_t handle = gattservice_subevent_spp_service_disconnected_get_con_handle(packet);
                    pbdrv_bluetooth_btstack_host_connection_t *host = pbdrv_bluetooth_btstack_get_host_connection(handle);
                    if (host) {
                        host->uart_configured = false;
                    }
                    break;
                }
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

    pbio_os_request_poll();
}

/**
 * Parses the HCI Read Local Version Information command response.
 *
 * @param [out] info      The device info to populate.
 * @param [in]  payload   The payload of the HCI event.
 */
static void parse_hci_local_version_information(pbdrv_bluetooth_btstack_local_version_info_t *info, const uint8_t *payload) {

    info->hci_version = payload[1];
    info->hci_revision = pbio_get_uint16_le(&payload[2]);
    info->lmp_pal_version = payload[4];
    info->manufacturer = pbio_get_uint16_le(&payload[5]);
    info->lmp_pal_subversion = pbio_get_uint16_le(&payload[7]);

    #if DEBUG
    // Show version in ev3dev format.
    uint16_t chip = (info->lmp_pal_subversion & 0x7C00) >> 10;
    uint16_t min_ver = (info->lmp_pal_subversion & 0x007F);
    uint16_t maj_ver = (info->lmp_pal_subversion & 0x0380) >> 7;
    if (info->lmp_pal_subversion & 0x8000) {
        maj_ver |= 0x0008;
    }
    DEBUG_PRINT("LMP %04x: TIInit_%d.%d.%d.bts\n", info->lmp_pal_subversion, chip, maj_ver, min_ver);
    #endif
}

static const pbdrv_bluetooth_btstack_chipset_info_t *chipset_info;

static bool pbdrv_bluetooth_btstack_ble_supported(void) {
    return chipset_info && chipset_info->supports_ble;
}

// Defined alongside the scan and connect thread below.
static void scan_and_connect_match_advertising_report(pbio_bluetooth_peripheral_t *peri, uint8_t *packet);

/**
 * Main handler for HCI and BLE events, recording state for the driver
 * thread to consume. Events of dedicated subsystems (security manager, GATT
 * client operations, inquiry scan) have their own handlers below.
 */
static void main_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

    // Platform-specific platform handler has priority.
    pbdrv_bluetooth_btstack_platform_packet_handler(packet_type, channel, packet, size);

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_COMMAND_COMPLETE: {
            const uint8_t *rp = hci_event_command_complete_get_return_parameters(packet);
            switch (hci_event_command_complete_get_command_opcode(packet)) {
                case HCI_OPCODE_HCI_READ_LOCAL_VERSION_INFORMATION: {
                    pbdrv_bluetooth_btstack_local_version_info_t info;
                    parse_hci_local_version_information(&info, rp);
                    chipset_info = pbdrv_bluetooth_btstack_set_chipset(&info);
                    break;
                }
                case HCI_OPCODE_HCI_LE_SET_ADVERTISE_ENABLE:
                    recorded_events.advertise_enable_complete = true;
                    pbio_os_request_poll();
                    break;
                case HCI_OPCODE_HCI_LE_SET_ADVERTISING_DATA:
                    recorded_events.advertising_data_complete = true;
                    pbio_os_request_poll();
                    break;
                default:
                    break;
            }
            break;
        }
        case GATT_EVENT_NOTIFICATION:
            for (uint8_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
                pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_peripheral_get_by_index(i);
                if (!peri || !peri->config.notification_handler) {
                    continue;
                }
                if (gatt_event_notification_get_handle(packet) == peri->con_handle) {
                    uint16_t length = gatt_event_notification_get_value_length(packet);
                    const uint8_t *value = gatt_event_notification_get_value(packet);
                    peri->config.notification_handler(peri->user, value, length);
                }
            }
            break;
        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                break;
            }
            // HCI_ROLE_SLAVE means the connecting device is the central and the hub is the peripheral
            // HCI_ROLE_MASTER means the connecting device is the peripheral and the hub is the central.
            if (hci_subevent_le_connection_complete_get_role(packet) == HCI_ROLE_SLAVE) {
                uint16_t handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                pbdrv_bluetooth_btstack_host_connection_t *host = pbdrv_bluetooth_btstack_get_host_connection(HCI_CON_HANDLE_INVALID);
                if (host == NULL) {
                    DEBUG_PRINT("Warning: more than %d LE connections established.\n", PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS);
                    break;
                }
                host->con_handle = handle;

                // don't start advertising again on disconnect
                gap_advertisements_enable(false);
                pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;
            } else if (recorded_events.scanning_peri &&
                       hci_subevent_le_connection_complete_get_status(packet) == ERROR_CODE_SUCCESS) {
                // Hub is central: record handle for the scan and connect thread.
                recorded_events.scanning_peri->con_handle =
                    hci_subevent_le_connection_complete_get_connection_handle(packet);
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            DEBUG_PRINT("HCI_EVENT_DISCONNECTION_COMPLETE\n");
            uint16_t handle = hci_event_disconnection_complete_get_connection_handle(packet);
            pbdrv_bluetooth_btstack_host_connection_t *host = pbdrv_bluetooth_btstack_get_host_connection(handle);
            if (host) {
                host->con_handle = HCI_CON_HANDLE_INVALID;
                host->pybricks_configured = false;
                host->uart_configured = false;
                pbio_bluetooth_host_connection_changed();
                DEBUG_PRINT("Host with handle %u disconnected\n", handle);
            } else {
                for (uint8_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
                    pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_peripheral_get_by_index(i);
                    if (peri && handle == peri->con_handle) {
                        DEBUG_PRINT("Peripheral %u with handle %u disconnected\n", i, peri->con_handle);
                        gatt_client_stop_listening_for_characteristic_value_updates(&peri->platform_state->notification);
                        peri->con_handle = HCI_CON_HANDLE_INVALID;
                        break;
                    }
                }
            }
            break;
        }
        case GAP_EVENT_ADVERTISING_REPORT: {
            uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
            uint8_t data_length = gap_event_advertising_report_get_data_length(packet);
            const uint8_t *data = gap_event_advertising_report_get_data(packet);

            if (pbdrv_bluetooth_observe_callback) {
                int8_t rssi = gap_event_advertising_report_get_rssi(packet);
                pbdrv_bluetooth_observe_callback(event_type, data, data_length, rssi);
            }

            if (recorded_events.scanning_peri) {
                scan_and_connect_match_advertising_report(recorded_events.scanning_peri, packet);
            }

            #if DEBUG
            bd_addr_t address;
            gap_event_advertising_report_get_address(packet, address);
            DEBUG_PRINT("GAP_EVENT_ADVERTISING_REPORT from addr %s type %d len %d\n", bd_addr_to_str(address), event_type, data_length);
            #endif

            break;
        }
        default:
            break;
    }

    pbio_os_request_poll();
}

// Security manager callbacks. This is adapted from the BTstack examples.
static void sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

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
        case SM_EVENT_PAIRING_COMPLETE: {
            DEBUG_PRINT("Pairing complete with status %u and reason %u.\n",
                sm_event_pairing_complete_get_status(packet),
                sm_event_pairing_complete_get_reason(packet));
            pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_btstack_get_peripheral(sm_event_pairing_complete_get_handle(packet));
            if (peri) {
                peri->platform_state->pairing_via_reencryption = false;
                peri->platform_state->pairing_status = sm_event_pairing_complete_get_status(packet);
                peri->platform_state->pairing_complete = true;
            }
            break;
        }
        case SM_EVENT_REENCRYPTION_STARTED: {
            bd_addr_t addr;
            sm_event_reencryption_complete_get_address(packet, addr);
            DEBUG_PRINT("Bonding information exists for addr type %u, identity addr %s -> start re-encryption\n",
                sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        }
        case SM_EVENT_REENCRYPTION_COMPLETE: {
            DEBUG_PRINT("Re-encryption complete with status %u.\n",
                sm_event_reencryption_complete_get_status(packet));
            pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_btstack_get_peripheral(sm_event_reencryption_complete_get_handle(packet));
            if (peri) {
                peri->platform_state->pairing_via_reencryption = true;
                peri->platform_state->pairing_status = sm_event_reencryption_complete_get_status(packet);
                peri->platform_state->pairing_complete = true;
            }
            break;
        }
        default:
            break;
    }

    pbio_os_request_poll();
}

/**
 * Handles events from GATT client operations on peripherals, recording the
 * results for the driver thread to consume. Results are keyed to a
 * peripheral by connection handle.
 */
static void gatt_op_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(packet_type);
    UNUSED(channel);
    UNUSED(size);

    switch (hci_event_packet_get_type(packet)) {
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
            pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_btstack_get_peripheral(
                gatt_event_characteristic_query_result_get_handle(packet));
            // During discovery, save only the first characteristic that has
            // at least the requested properties.
            if (!peri || peri->char_disc.handle) {
                break;
            }
            gatt_client_characteristic_t *current_char = &peri->platform_state->current_char;
            gatt_event_characteristic_query_result_get_characteristic(packet, current_char);
            if ((current_char->properties & peri->char_disc.properties) == peri->char_disc.properties) {
                DEBUG_PRINT("Found characteristic handle: 0x%04x with properties 0x%04x \n", current_char->value_handle, current_char->properties);
                peri->char_disc.handle = current_char->value_handle;
                peri->char_disc.handle_max = current_char->end_handle;
            }
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT: {
            pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_btstack_get_peripheral(
                gatt_event_characteristic_value_query_result_get_handle(packet));
            if (peri && gatt_event_characteristic_value_query_result_get_value_handle(packet) == peri->char_handle) {
                peri->char_size = gatt_event_characteristic_value_query_result_get_value_length(packet);
                memcpy(peri->char_data, gatt_event_characteristic_value_query_result_get_value(packet), peri->char_size);
            }
            break;
        }
        case GATT_EVENT_QUERY_COMPLETE: {
            pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_btstack_get_peripheral(
                gatt_event_query_complete_get_handle(packet));
            if (peri) {
                peri->platform_state->gatt_att_status = gatt_event_query_complete_get_att_status(packet);
                peri->platform_state->gatt_query_complete = true;
            }
            break;
        }
        default:
            break;
    }

    pbio_os_request_poll();
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
    gap_advertisements_set_params(0x30, 0x30, PBIO_BLUETOOTH_AD_TYPE_ADV_IND, 0x00, null_addr, 0x07, 0x00);

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
        // PnP ID characteristic UUID, followed by the PnP ID value.
        PBIO_UINT16_LE(PBIO_GATT_PNP_ID_CHAR_UUID), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    };

    pbio_pybricks_pnp_id(&scan_resp_data[4], PBDRV_CONFIG_HUB_KIND, PBDRV_CONFIG_HUB_VARIANT);

    uint8_t hub_name_len = strlen(pbdrv_bluetooth_hub_name);
    scan_resp_data[11] = hub_name_len + 1;
    memcpy(&scan_resp_data[13], pbdrv_bluetooth_hub_name, hub_name_len);
    _Static_assert(13 + sizeof(pbdrv_bluetooth_hub_name) - 1 <= 31, "scan response is 31 octet max");

    gap_scan_response_set_data(13 + hub_name_len, scan_resp_data);
}

pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context) {
    #if !PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    return PBIO_ERROR_NOT_SUPPORTED;
    #endif

    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_bluetooth_btstack_host_connection_t *host = pbdrv_bluetooth_btstack_get_host_connection(HCI_CON_HANDLE_INVALID);
    if (host == NULL) {
        // There should be at least one available host connection. Otherwise
        // we will never receive the advertise completion event below.
        return PBIO_ERROR_INVALID_OP;
    }

    init_advertising_data();
    recorded_events.advertise_enable_complete = false;
    gap_advertisements_enable(true);

    PBIO_OS_AWAIT_UNTIL(state, recorded_events.advertise_enable_complete);

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_ADVERTISING_PYBRICKS;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    gap_advertisements_enable(false);

    // REVISIT: use callback to await operation

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
static void pybricks_on_ready_to_send(void *context) {
    pbdrv_bluetooth_btstack_host_connection_t *host = context;
    pybricks_service_server_send(host->con_handle, host->notification_data, host->notification_size);
    host->notification_done = true;
    pbio_os_request_poll();
}
#endif

uint16_t pbdrv_bluetooth_get_max_message_size(void) {
    // Minimum negotiated ATT MTU over all connected hosts, capped at the
    // platform maximum, minus the 3-byte ATT notification header. The same
    // value is sent to every host, so we must fit the smallest one.
    uint16_t mtu = PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE;
    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    for (size_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS; i++) {
        pbdrv_bluetooth_btstack_host_connection_t *host = &host_connections[i];
        if (host->con_handle == HCI_CON_HANDLE_INVALID) {
            continue;
        }
        uint16_t host_mtu = att_server_get_mtu(host->con_handle);
        if (host_mtu && host_mtu < mtu) {
            mtu = host_mtu;
        }
    }
    #endif
    return mtu - PBIO_BLUETOOTH_ATT_HEADER_SIZE;
}

pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    static size_t i;
    static pbdrv_bluetooth_btstack_host_connection_t *host;

    PBIO_OS_ASYNC_BEGIN(state);

    for (i = 0; i < PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS; i++) {
        host = &host_connections[i];
        if (host->con_handle != HCI_CON_HANDLE_INVALID && host->pybricks_configured) {
            host->notification_data = data;
            host->notification_size = size;
            host->notification_done = false;
            pybricks_service_server_request_can_send_now(&host->send_request, host->con_handle);
        } else {
            // No connection or not configured, don't hold up wait loop below.
            host->notification_done = true;
        }
    }

    // Wait for all notifications to be sent. BTstack handles sending
    // asynchronously. This loop completes when the slowest one is done.
    // Stop waiting if we get disconnected in the middle of it.
    for (i = 0; i < PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS; i++) {
        host = &host_connections[i];
        PBIO_OS_AWAIT_UNTIL(state, host->notification_done || host->con_handle == HCI_CON_HANDLE_INVALID);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
    #else
    return PBIO_ERROR_NOT_SUPPORTED;
    #endif
}

/**
 * Matches one advertising report against the filters of the peripheral
 * currently scanning. Called by the packet handler on each report while the
 * scan and connect thread below awaits the scan_rsp_matched result.
 *
 * First matches an advertisement and records the sender address, then waits
 * for the scan response from that same address. If the scan response does
 * not pass the filter, goes back to matching advertisements, but not the
 * device that was just rejected.
 */
static void scan_and_connect_match_advertising_report(pbio_bluetooth_peripheral_t *peri, uint8_t *packet) {

    uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
    uint8_t data_length = gap_event_advertising_report_get_data_length(packet);
    const uint8_t *data = gap_event_advertising_report_get_data(packet);

    if (!peri->platform_state->adv_matched) {
        // Match advertisement data against context-specific filter.
        if (event_type <= PBIO_BLUETOOTH_AD_TYPE_ADV_DIRECT_IND &&
            peri->config.match_adv(peri->user, data, data_length)) {
            bd_addr_t adv_address;
            gap_event_advertising_report_get_address(packet, adv_address);
            // If it is the same device as before, its scan response
            // didn't match earlier, so don't try it again.
            bool saw_before = !memcmp(peri->bdaddr, adv_address, sizeof(bd_addr_t));
            memcpy(peri->bdaddr, adv_address, sizeof(bd_addr_t));
            peri->bdaddr_type = gap_event_advertising_report_get_address_type(packet);
            if (!saw_before) {
                // Copy data to allow virtual re-connect in a new user program.
                peri->config.match_adv_data_len = data_length;
                memcpy(peri->config.match_adv_data, data, data_length);
                peri->platform_state->adv_matched = true;
                DEBUG_PRINT("Advertisement matched, waiting for scan response\n");
            }
        }
    } else if (!peri->platform_state->scan_rsp_matched &&
               event_type == PBIO_BLUETOOTH_AD_TYPE_SCAN_RSP) {
        bd_addr_t rsp_address;
        gap_event_advertising_report_get_address(packet, rsp_address);
        if (!memcmp(peri->bdaddr, rsp_address, sizeof(bd_addr_t))) {
            if (!peri->config.match_adv_rsp(peri->user, data, data_length)) {
                // Scan response from the matched device did not pass
                // the response filter (e.g. requested name did not
                // match), so go back to matching advertisements.
                DEBUG_PRINT("Name requested but did not match. Scan again.\n");
                peri->platform_state->adv_matched = false;
            } else {
                // Copy name for later use.
                if (data[1] == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) {
                    memcpy(peri->name, &data[2], sizeof(peri->name));
                }
                // Copy response data to allow virtual re-connect in a new user program.
                peri->config.match_adv_rsp_data_len = data_length;
                memcpy(peri->config.match_adv_rsp_data, data, data_length);
                peri->platform_state->scan_rsp_matched = true;
            }
        }
    }
}

pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbio_bluetooth_peripheral_t *peri = context;
    uint8_t btstack_error;

    // Operation can be explicitly cancelled or automatically on inactivity.
    if (!peri->cancel) {
        peri->cancel = pbio_os_timer_is_expired(&peri->watchdog);
    }
    bool scan_timed_out = peri->config.timeout && pbio_os_timer_is_expired(&peri->timer);

    PBIO_OS_ASYNC_BEGIN(state);

    memset(peri->platform_state, 0, sizeof(pbio_bluetooth_peripheral_platform_state_t));

    peri->con_handle = HCI_CON_HANDLE_INVALID;
    recorded_events.scanning_peri = peri;

    // active scanning to get scan response data.
    // scan interval: 48 * 0.625ms = 30ms
    gap_set_scan_params(1, 0x30, 0x30, 0);
    gap_start_scan();

    // The packet handler matches advertisements and scan responses against
    // the filters and records the results, retrying other devices as needed.
    // See scan_and_connect_match_advertising_report() which ultimately sets
    // this state to proceed.
    PBIO_OS_AWAIT_UNTIL(state, scan_timed_out || peri->cancel || peri->platform_state->scan_rsp_matched);

    if (scan_timed_out || peri->cancel) {
        DEBUG_PRINT("Scan %s.\n", peri->cancel ? "canceled": "timed out");
        recorded_events.scanning_peri = NULL;
        gap_stop_scan();
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    DEBUG_PRINT("Scan response matched, initiate connection to %s.\n", bd_addr_to_str(peri->bdaddr));

    // We can stop scanning now.
    gap_stop_scan();

    // Initiate connection and await connection complete event.
    // The user timeout applies only to finding the device. We still want to
    // have a reasonable timeout for connecting and pairing.
    pbio_os_timer_set(&peri->timer, PERIPHERAL_TIMEOUT_MS_CONNECT);
    btstack_error = gap_connect(peri->bdaddr, peri->bdaddr_type);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        recorded_events.scanning_peri = NULL;
        return att_error_to_pbio_error(btstack_error);
    }
    PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&peri->timer) || peri->cancel ||
        pbdrv_bluetooth_peripheral_is_connected(peri));

    recorded_events.scanning_peri = NULL;

    // If we timed out or were cancelled, abort the connection. We have to
    // check the connection again in case cancellation and connection
    // completed simultaneously.
    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        DEBUG_PRINT("Connection %s.\n", peri->cancel ? "canceled": "timed out");
        gap_connect_cancel();
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    DEBUG_PRINT("Connected with handle %d.\n", peri->con_handle);

    // We are done if no pairing is requested.
    if (!(peri->config.options & PBIO_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR)) {
        DEBUG_PRINT("Simple connection done.\n");
        return PBIO_SUCCESS;
    }

start_pairing:

    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        DEBUG_PRINT("Not connected anymore, cannot pair.\n");
        return PBIO_ERROR_NO_DEV;
    }

    // Re-encryption doesn't seem to work reliably, so we always delete the
    // bond and start over. REVISIT: We should be able to catch this now, so
    // allow re-encryption to succeed.
    DEBUG_PRINT("Request pairing.\n");
    pbio_os_timer_set(&peri->timer, PERIPHERAL_TIMEOUT_MS_PAIRING);
    gap_delete_bonding(peri->bdaddr_type, peri->bdaddr);
    peri->platform_state->pairing_complete = false;
    sm_request_pairing(peri->con_handle);

    // Wait for pairing to complete unless timed out, cancelled, or disconnected.
    PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&peri->timer) || peri->cancel ||
        !pbdrv_bluetooth_peripheral_is_connected(peri) ||
        peri->platform_state->pairing_complete);

    // If we timed out or were cancelled, disconnect and leave.
    if (!peri->platform_state->pairing_complete) {
        if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
            DEBUG_PRINT("Not connected anymore, cannot complete pairing.\n");
            return PBIO_ERROR_NO_DEV;
        }
        DEBUG_PRINT("Bonding %s.\n", peri->cancel ? "canceled": "timed out");
        pbdrv_bluetooth_peripheral_disconnect_now(peri);
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    // Pairing ended, successfully or not, either as a new pairing or re-encryption.
    // Test re-encryption result first.
    btstack_error = peri->platform_state->pairing_status;
    if (peri->platform_state->pairing_via_reencryption) {
        DEBUG_PRINT("Re-encryption complete, bonded device with error %u.\n", btstack_error);

        if (btstack_error == ERROR_CODE_SUCCESS) {
            DEBUG_PRINT("Re-encryption successful, bonded device.\n");
            return PBIO_SUCCESS;
        }

        switch (btstack_error) {
            case ERROR_CODE_PIN_OR_KEY_MISSING:
                DEBUG_PRINT("Bonding information missing.\n");
                break;
            case ERROR_CODE_CONNECTION_FAILED_TO_BE_ESTABLISHED:
            // fallthrough
            case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                DEBUG_PRINT("Re-encrytion failed. Did remote disconnect?\n");
                break;
            default:
                DEBUG_PRINT("Other re-encryption failure.\n");
                break;
        }

        DEBUG_PRINT("Deleting bond for %s and retrying.\n", bd_addr_to_str(peri->bdaddr));
        gap_delete_bonding(peri->bdaddr_type, peri->bdaddr);
        goto start_pairing;
    }

    // Otherwise we have received pairing complete. Check status and disconnect
    // on failure.
    DEBUG_PRINT("New pairing completed with error %u.\n", btstack_error);

    if (btstack_error == ERROR_CODE_SUCCESS) {
        DEBUG_PRINT("Pairing successful.\n");
        return PBIO_SUCCESS;
    }

    switch (btstack_error) {
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
            DEBUG_PRINT("Did remote disconnect?\n");
            break;
        default:
            DEBUG_PRINT("Other pairing failure.\n");
            break;
    }

    pbdrv_bluetooth_peripheral_disconnect_now(peri);
    return att_error_to_pbio_error(btstack_error);

    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbio_bluetooth_peripheral_t *peri = context;
    gatt_client_characteristic_t *current_char = &peri->platform_state->current_char;

    uint8_t btstack_error;

    PBIO_OS_ASYNC_BEGIN(state);

    // Nothing found to begin with.
    peri->char_disc.handle = 0;

    uint16_t handle_max = peri->char_disc.handle_max ? peri->char_disc.handle_max : 0xffff;

    // Start characteristic discovery. The handler processes each discovered
    // characteristic to see if it matches what we need.
    peri->platform_state->gatt_query_complete = false;
    btstack_error = peri->char_disc.uuid16 ?
        gatt_client_discover_characteristics_for_handle_range_by_uuid16(
        gatt_op_handler, peri->con_handle, 0x0001, handle_max, peri->char_disc.uuid16) :
        gatt_client_discover_characteristics_for_handle_range_by_uuid128(
        gatt_op_handler, peri->con_handle, 0x0001, handle_max, peri->char_disc.uuid128);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    DEBUG_PRINT("Discovering characteristic\n");
    PBIO_OS_AWAIT_UNTIL(state, !pbdrv_bluetooth_peripheral_is_connected(peri) ||
        peri->platform_state->gatt_query_complete);

    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        // Got disconnected while waiting.
        return PBIO_ERROR_NO_DEV;
    }

    // Result of discovery.
    btstack_error = peri->platform_state->gatt_att_status;
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    if (!peri->char_disc.handle) {
        // Characteristic not found.
        return PBIO_ERROR_FAILED;
    }

    // If no notification is requested, we are done.
    if (!peri->char_disc.request_notification) {
        // Success if we found the characteristic.
        return peri->char_disc.handle ? PBIO_SUCCESS : PBIO_ERROR_FAILED;
    }

    // Discovered characteristics, ready to enable notifications.
    peri->platform_state->gatt_query_complete = false;
    btstack_error = gatt_client_write_client_characteristic_configuration(
        gatt_op_handler, peri->con_handle, current_char, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);

    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    DEBUG_PRINT("Waiting for notifications to be enabled.\n");
    PBIO_OS_AWAIT_UNTIL(state, !pbdrv_bluetooth_peripheral_is_connected(peri) ||
        peri->platform_state->gatt_query_complete);

    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        // Got disconnected while waiting.
        return PBIO_ERROR_NO_DEV;
    }

    // Result of enabling notifications.
    btstack_error = peri->platform_state->gatt_att_status;
    if (btstack_error != ERROR_CODE_SUCCESS) {
        // ATT_ERROR_INSUFFICIENT_AUTHENTICATION
        return att_error_to_pbio_error(btstack_error);
    }

    // Register notification handler.
    gatt_client_listen_for_characteristic_value_updates(
        &peri->platform_state->notification, main_packet_handler, peri->con_handle, current_char);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbio_bluetooth_peripheral_t *peri = context;

    uint8_t btstack_error;

    PBIO_OS_ASYNC_BEGIN(state);

    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        return PBIO_ERROR_NO_DEV;
    }

    gatt_client_characteristic_t characteristic = {
        .value_handle = peri->char_handle,
    };
    // The handler caches the value result as it comes in.
    peri->platform_state->gatt_query_complete = false;
    btstack_error = gatt_client_read_value_of_characteristic(gatt_op_handler, peri->con_handle, &characteristic);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    PBIO_OS_AWAIT_UNTIL(state, !pbdrv_bluetooth_peripheral_is_connected(peri) ||
        peri->platform_state->gatt_query_complete);

    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        // Got disconnected while waiting.
        return PBIO_ERROR_NO_DEV;
    }

    // Result of read operation.
    btstack_error = peri->platform_state->gatt_att_status;
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbio_bluetooth_peripheral_t *peri = context;

    uint8_t btstack_error;

    PBIO_OS_ASYNC_BEGIN(state);

    peri->platform_state->gatt_query_complete = false;
    btstack_error = gatt_client_write_value_of_characteristic(gatt_op_handler,
        peri->con_handle, peri->char_handle, peri->char_size, peri->char_data);

    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    PBIO_OS_AWAIT_UNTIL(state, !pbdrv_bluetooth_peripheral_is_connected(peri) ||
        peri->platform_state->gatt_query_complete);

    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        // Got disconnected while waiting.
        return PBIO_ERROR_NO_DEV;
    }

    // Result of write operation.
    btstack_error = peri->platform_state->gatt_att_status;
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbio_bluetooth_peripheral_t *peri = context;

    PBIO_OS_ASYNC_BEGIN(state);

    // If already disconnected, this completes immediately.
    pbdrv_bluetooth_peripheral_disconnect_now(peri);
    PBIO_OS_AWAIT_UNTIL(state, peri->con_handle == HCI_CON_HANDLE_INVALID);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    recorded_events.advertising_data_complete = false;
    gap_advertisements_set_data(pbdrv_bluetooth_broadcast_data_size, pbdrv_bluetooth_broadcast_data);

    // If already broadcasting, await set data and return.
    if (pbdrv_bluetooth_advertising_state == PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING) {
        PBIO_OS_AWAIT_UNTIL(state, recorded_events.advertising_data_complete);
        return PBIO_SUCCESS;
    }

    bd_addr_t null_addr = { };
    gap_advertisements_set_params(0xA0, 0xA0, PBIO_BLUETOOTH_AD_TYPE_ADV_NONCONN_IND, 0, null_addr, 0x7, 0);
    recorded_events.advertise_enable_complete = false;
    gap_advertisements_enable(true);

    PBIO_OS_AWAIT_UNTIL(state, recorded_events.advertise_enable_complete);

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    if (!pbdrv_bluetooth_is_observing) {
        gap_set_scan_params(0, 0x30, 0x30, 0);
        gap_start_scan();
        pbdrv_bluetooth_is_observing = true;
        // REVISIT: use callback to await operation
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    if (pbdrv_bluetooth_is_observing) {
        gap_stop_scan();
        pbdrv_bluetooth_is_observing = false;
        // REVISIT: use callback to await operation
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#ifdef ENABLE_CLASSIC

/**
 * Bluetooth Classic event outcomes recorded for the driver thread to consume.
 */
static struct {
    /**
     * Inquiry scan destination registered by the thread. Inquiry results are
     * recorded here. NULL when not scanning.
     */
    pbdrv_bluetooth_classic_task_context_t *inquiry_task;
    /** Inquiry scan completed. */
    bool inquiry_complete;
} classic_events;

static void pbdrv_bluetooth_inquiry_unpack_scan_event(uint8_t *event_packet, pbdrv_bluetooth_classic_task_context_t *task) {

    bd_addr_t bdaddr;
    gap_event_inquiry_result_get_bd_addr(event_packet, bdaddr);

    // Exit if already seen this.
    for (uint32_t i = 0; i < *task->inq_count; i++) {
        if (memcmp(task->inq_results[i].bdaddr, bdaddr, sizeof(bd_addr_t)) == 0) {
            return;
        }
    }

    // Unpack new device.
    pbio_bluetooth_inquiry_result_t *result = &task->inq_results[(*task->inq_count)++];
    memcpy(result->bdaddr, bdaddr, sizeof(bd_addr_t));

    if (gap_event_inquiry_result_get_rssi_available(event_packet)) {
        result->rssi = gap_event_inquiry_result_get_rssi(event_packet);
    }
    if (gap_event_inquiry_result_get_name_available(event_packet)) {
        const uint8_t *name = gap_event_inquiry_result_get_name(event_packet);
        const size_t name_len = gap_event_inquiry_result_get_name_len(event_packet);
        snprintf(result->name, sizeof(result->name), "%.*s", (int)name_len, name);
    }
    result->class_of_device = gap_event_inquiry_result_get_class_of_device(event_packet);

    DEBUG_PRINT("Received new scan result with name %s and type %d and rssi %d\n", result->name, result->class_of_device, result->rssi);

}

/**
 * Handles inquiry scan events, recording results into the registered task
 * for the driver thread to consume.
 */
static void inquiry_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }

    switch (hci_event_packet_get_type(packet)) {
        case GAP_EVENT_INQUIRY_RESULT: {
            pbdrv_bluetooth_classic_task_context_t *task = classic_events.inquiry_task;
            pbdrv_bluetooth_inquiry_unpack_scan_event(packet, task);
            break;
        }
        case GAP_EVENT_INQUIRY_COMPLETE:
            classic_events.inquiry_complete = true;
            break;
        default:
            return;
    }

    pbio_os_request_poll();
}

pbio_error_t pbdrv_bluetooth_inquiry_scan_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_classic_task_context_t *task = context;

    if (!task->cancel) {
        task->cancel = pbio_os_timer_is_expired(&task->watchdog);
    }

    PBIO_OS_ASYNC_BEGIN(state);

    DEBUG_PRINT("Start inquiry scan.\n");

    classic_events.inquiry_task = task;
    classic_events.inquiry_complete = false;
    gap_inquiry_start(task->inq_duration);

    // Wait until scan complete or the number of devices are found. The
    // handler records the results as they come in.
    PBIO_OS_AWAIT_UNTIL(state, task->cancel || *task->inq_count_max == 0 ||
        *task->inq_count == *task->inq_count_max || classic_events.inquiry_complete);

    classic_events.inquiry_task = NULL;

    if (task->cancel || *task->inq_count_max == 0) {
        // Cancelled or the external data no longer available.
        DEBUG_PRINT("Inquiry scan canceled.\n");
        gap_inquiry_stop();
        return PBIO_ERROR_CANCELED;
    }

    DEBUG_PRINT("Inquiry scan ended with %d results.\n", *task->inq_count);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#endif // ENABLE_CLASSIC

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

static pbio_error_t bluetooth_btstack_handle_power_control(pbio_os_state_t *state, HCI_POWER_MODE power_mode, HCI_STATE end_state) {

    PBIO_OS_ASYNC_BEGIN(state);

    // Events emitted synchronously by this call only record state, so
    // this is safe to call from within the Bluetooth process.
    hci_power_control(power_mode);

    // Wait for the power state to take effect.
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == end_state);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    if (!pbdrv_bluetooth_hci_is_enabled()) {
        return PBIO_ERROR_INVALID_OP;
    }

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // Wait for power off.
    PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_handle_power_control(&sub, HCI_POWER_OFF, HCI_STATE_OFF));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // Wait for power on.
    PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_handle_power_control(&sub, HCI_POWER_ON, HCI_STATE_WORKING));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_disconnect_all(pbio_os_state_t *state) {

    if (!pbdrv_bluetooth_hci_is_enabled()) {
        return PBIO_ERROR_INVALID_OP;
    }

    static pbio_os_state_t sub;
    static uint8_t i;
    static pbio_bluetooth_peripheral_t *peri;

    PBIO_OS_ASYNC_BEGIN(state);

    DEBUG_PRINT("Disconnecting all peripheral connections.\n");

    // Disconnect gracefully if connected to peripherals.
    #if PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS
    for (i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
        peri = pbdrv_bluetooth_peripheral_get_by_index(i);
        // Must call the platform specific function since this runs after
        // the Bluetooth main loop ends.
        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_peripheral_disconnect_func(&sub, peri));
    }
    #endif

    DEBUG_PRINT("Disconnecting all host connections.\n");

    // Disconnect gracefully if connected to hosts.
    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    static pbdrv_bluetooth_btstack_host_connection_t *host;
    for (i = 0; i < PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS; i++) {
        host = &host_connections[i];
        if (host->con_handle == HCI_CON_HANDLE_INVALID) {
            continue;
        }
        gap_disconnect(host->con_handle);
        PBIO_OS_AWAIT_UNTIL(state, host->con_handle == HCI_CON_HANDLE_INVALID);
    }
    #endif

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static void bluetooth_btstack_run_loop_set_timer(btstack_timer_source_t *ts, uint32_t timeout_in_ms) {
    ts->timeout = pbdrv_clock_get_ms() + timeout_in_ms;
}

static void bluetooth_btstack_run_loop_execute(void) {
    // not used
}

/**
 * The event loop runs on every system poll, but we only need to go through
 * the btstack data sources when explicitly requested.
 */
static volatile bool pbdrv_bluetooth_btstack_poll_requested;

static void pbdrv_bluetooth_btstack_run_loop_trigger(void) {
    pbdrv_bluetooth_btstack_poll_requested = true;
    pbio_os_request_poll();
}

static const btstack_run_loop_t bluetooth_btstack_run_loop = {
    .init = btstack_run_loop_base_init,
    .add_data_source = btstack_run_loop_base_add_data_source,
    .remove_data_source = btstack_run_loop_base_remove_data_source,
    .enable_data_source_callbacks = btstack_run_loop_base_enable_data_source_callbacks,
    .disable_data_source_callbacks = btstack_run_loop_base_disable_data_source_callbacks,
    .set_timer = bluetooth_btstack_run_loop_set_timer,
    .add_timer = btstack_run_loop_base_add_timer,
    .remove_timer = btstack_run_loop_base_remove_timer,
    .execute = bluetooth_btstack_run_loop_execute,
    .dump_timer = btstack_run_loop_base_dump_timer,
    .get_time_ms = pbdrv_clock_get_ms,
    .poll_data_sources_from_irq = pbdrv_bluetooth_btstack_run_loop_trigger,
};

static pbio_os_process_t pbdrv_bluetooth_hci_process;

/**
 * This process is slightly unusual in that it does not have a state. It is
 * essentially just a poll handler.
 */
static pbio_error_t pbdrv_bluetooth_hci_process_thread(pbio_os_state_t *state, void *context) {

    if (pbdrv_bluetooth_btstack_poll_requested) {
        pbdrv_bluetooth_btstack_poll_requested = false;
        btstack_run_loop_base_poll_data_sources();
    }

    pbdrv_bluetooth_btstack_platform_poll();

    static pbio_os_timer_t btstack_timer = {
        .duration = 1,
    };

    if (pbio_os_timer_is_expired(&btstack_timer)) {
        pbio_os_timer_extend(&btstack_timer);
        btstack_run_loop_base_process_timers(pbdrv_clock_get_ms());
    }

    // Drive the common Bluetooth process. All events received above have been
    // recorded as state by the packet handlers for the process to consume.
    if (bluetooth_thread_err == PBIO_ERROR_AGAIN) {
        bluetooth_thread_err = pbdrv_bluetooth_process_thread(&bluetooth_thread_state, NULL);
    }

    return bluetooth_thread_err;
}

// Have to be defined, but are not used.
const uint32_t cc256x_init_script_size = 0;
const uint8_t cc256x_init_script[] = {};

void pbdrv_bluetooth_init(void) {

    // Initialize host structs as not connected.
    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    for (size_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS; i++) {
        pbdrv_bluetooth_btstack_host_connection_t *host = &host_connections[i];
        host->con_handle = HCI_CON_HANDLE_INVALID;
        host->send_request.callback = pybricks_on_ready_to_send;
        host->send_request.context = host;
    }
    #endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS

    // Proceed to start Bluetooth process only if platform init passes.
    pbio_error_t err = pbdrv_bluetooth_btstack_platform_init();
    if (err != PBIO_SUCCESS) {
        return;
    }

    #if DEBUG == 2
    hci_dump_init(&pbdrv_bluetooth_btstack_hci_dump);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_INFO, true);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_ERROR, true);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_DEBUG, true);
    #endif

    static btstack_packet_callback_registration_t hci_event_callback_registration;

    // Attach btstack platform state to peripherals.
    for (uint8_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
        pbio_bluetooth_peripheral_t *peri = pbdrv_bluetooth_peripheral_get_by_index(i);
        peri->platform_state = &peripheral_platform_state[i];
        peri->con_handle = HCI_CON_HANDLE_INVALID;
    }

    btstack_memory_init();
    btstack_run_loop_init(&bluetooth_btstack_run_loop);

    // Chipset is not set here. This is done dynamically when we get the
    // local version information.
    hci_init(pdata->transport_instance(), pdata->transport_config());
    hci_set_control(pdata->control_instance());

    // REVISIT: do we need to call btstack_chipset_cc256x_set_power() or btstack_chipset_cc256x_set_power_vector()?

    hci_event_callback_registration.callback = &main_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    #ifdef ENABLE_CLASSIC
    // Needed to get device names.
    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    static btstack_packet_callback_registration_t inquiry_event_callback_registration;
    inquiry_event_callback_registration.callback = &inquiry_packet_handler;
    hci_add_event_handler(&inquiry_event_callback_registration);
    #endif

    l2cap_init();

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

    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_NUM_LE_HOSTS
    // setup ATT server
    att_server_init(profile_data, att_read_callback, NULL);

    device_information_service_server_init();
    device_information_service_server_set_firmware_revision(PBIO_VERSION_STR);
    device_information_service_server_set_software_revision(PBIO_PROTOCOL_VERSION_STR);
    uint8_t pnp_id[PBIO_PYBRICKS_PNP_ID_SIZE];
    pbio_pybricks_pnp_id(pnp_id, PBDRV_CONFIG_HUB_KIND, PBDRV_CONFIG_HUB_VARIANT);
    device_information_service_server_set_pnp_id(pnp_id[0],
        pbio_get_uint16_le(&pnp_id[1]), pbio_get_uint16_le(&pnp_id[3]), pbio_get_uint16_le(&pnp_id[5]));

    pybricks_service_server_init(pybricks_data_received, pybricks_configured);
    nordic_spp_service_server_init(nordic_spp_packet_handler);
    #else
    (void)pybricks_data_received;
    (void)att_read_callback;
    (void)pybricks_configured;
    (void)nordic_spp_packet_handler;
    (void)sm_packet_handler;
    #endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE

    bluetooth_thread_err = PBIO_ERROR_AGAIN;
    bluetooth_thread_state = 0;
    pbio_os_process_start(&pbdrv_bluetooth_hci_process, pbdrv_bluetooth_hci_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK
