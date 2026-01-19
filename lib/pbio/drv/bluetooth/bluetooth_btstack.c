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

#include <bluetooth_sdp.h>
#include <classic/rfcomm.h>
#include <classic/sdp_client.h>
#include <classic/sdp_server.h>
#include <classic/spp_server.h>
#include <pbsys/storage.h>
#include <pbsys/storage_settings.h>
#include <lwrb/lwrb.h>

#if HAVE_UMM_MALLOC
#include <umm_malloc.h>
#endif

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>

#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/version.h>

#include "bluetooth.h"
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

// Timeouts for various steps in the scan and connect process.
#define PERIPHERAL_TIMEOUT_MS_SCAN_RESPONSE (2000)
#define PERIPHERAL_TIMEOUT_MS_CONNECT       (5000)
#define PERIPHERAL_TIMEOUT_MS_PAIRING       (5000)

#define DEBUG 2

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
static struct _pbdrv_bluetooth_peripheral_platform_state_t {
    /**
     * Current connection state, needed to unset callback on disconnect.
     */
    gatt_client_notification_t notification;
    /**
     *  Character information used during discovery. Assuming properties and chars
     *  are set up such that only one char is discovered at a time
     */
    gatt_client_characteristic_t current_char;
} peripheral_platform_state[PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS];

static pbdrv_bluetooth_peripheral_t _peripherals[PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS];

pbdrv_bluetooth_peripheral_t *pbdrv_bluetooth_peripheral_get_by_index(uint8_t index) {
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

static uint8_t *event_packet;
static const pbdrv_bluetooth_btstack_platform_data_t *pdata = &pbdrv_bluetooth_btstack_platform_data;
static hci_con_handle_t le_con_handle = HCI_CON_HANDLE_INVALID;
static hci_con_handle_t pybricks_con_handle = HCI_CON_HANDLE_INVALID;
static hci_con_handle_t uart_con_handle = HCI_CON_HANDLE_INVALID;

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
        case BTSTACK_MEMORY_ALLOC_FAILED:
            DEBUG_PRINT("BTstack out of memory.\n");
        // fallthrough
        default:
            DEBUG_PRINT("Failed with status.%d\n", status);
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

static bool hci_event_is_type(uint8_t *packet, uint8_t event_type) {
    return packet && hci_event_packet_get_type(packet) == event_type;
}

/**
 * Shortcut to check if a LE connection event for a peripheral role occurred.
 */
static bool hci_event_le_peripheral_did_connect(uint8_t *packet) {
    return
        hci_event_is_type(event_packet, HCI_EVENT_LE_META) &&
        hci_event_le_meta_get_subevent_code(event_packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE &&
        hci_subevent_le_connection_complete_get_role(event_packet) == HCI_ROLE_MASTER;
}

/**
 * Shortcut to check if a pairing or reencryption complete event for a given
 * handle occurred. Indicates end of pairing process.
 */
static bool hci_event_le_peripheral_pairing_did_complete(uint8_t *packet, uint16_t handle) {
    if (hci_event_is_type(event_packet, SM_EVENT_PAIRING_COMPLETE)) {
        return sm_event_pairing_complete_get_handle(event_packet) == handle;
    }
    if (hci_event_is_type(event_packet, SM_EVENT_REENCRYPTION_COMPLETE)) {
        return sm_event_reencryption_complete_get_handle(event_packet) == handle;
    }
    return false;
}

/**
 * Wrapper for gap_disconnect that is safe to call if already disconnected.
 */
static void pbdrv_bluetooth_peripheral_disconnect_now(pbdrv_bluetooth_peripheral_t *peri) {
    if (peri->con_handle == HCI_CON_HANDLE_INVALID) {
        // Already disconnected. We must check this because otherwise
        // gap_disconnect() will synchronously call the disconnection complete
        // handler and re-enter the event loop recursively.
        return;
    }
    gap_disconnect(peri->con_handle);
}

/**
 * Checks if the given peripheral is connected.
 */
bool pbdrv_bluetooth_peripheral_is_connected(pbdrv_bluetooth_peripheral_t *peri) {
    return peri->con_handle != HCI_CON_HANDLE_INVALID;
}

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

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {

    // Nothing connected if HCI is not running.
    if (bluetooth_thread_err != PBIO_ERROR_AGAIN) {
        return false;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_HCI) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_LE && le_con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS && pybricks_con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_UART && uart_con_handle != HCI_CON_HANDLE_INVALID) {
        return true;
    }

    return false;
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
            // Not implemented.
            break;
        default:
            break;
    }

    propagate_event(packet);
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

static void maybe_handle_classic_security_packet(uint8_t *packet, uint16_t size);

// currently, this function just handles the Powered Up handset control.
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {

    // Platform-specific platform handler has priority.
    pbdrv_bluetooth_btstack_platform_packet_handler(packet_type, channel, packet, size);

    if (packet_type == HCI_EVENT_PACKET) {
        // We have a separate handler for classic security packets,
        // which we don't mix in here for clarity's sake.
        maybe_handle_classic_security_packet(packet, size);
    }

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
                default:
                    break;
            }
            break;
        }
        case GATT_EVENT_SERVICE_QUERY_RESULT: {
            // Service discovery not used.
            gatt_client_service_t service;
            gatt_event_service_query_result_get_service(packet, &service);
            break;
        }
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            DEBUG_PRINT("GATT_EVENT_CHARACTERISTIC_QUERY_RESULT\n");
            break;
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT: {
            DEBUG_PRINT("GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT\n");
            break;
        }
        case GATT_EVENT_QUERY_COMPLETE:
            DEBUG_PRINT("GATT_EVENT_QUERY_COMPLETE\n");
            break;
        case GATT_EVENT_NOTIFICATION:
            for (uint8_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
                pbdrv_bluetooth_peripheral_t *peri = pbdrv_bluetooth_peripheral_get_by_index(i);
                if (!peri || !peri->config || !peri->config->notification_handler) {
                    continue;
                }
                if (gatt_event_notification_get_handle(packet) == peri->con_handle) {
                    uint16_t length = gatt_event_notification_get_value_length(packet);
                    const uint8_t *value = gatt_event_notification_get_value(packet);
                    peri->config->notification_handler(peri->user, value, length);
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
                le_con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                // don't start advertising again on disconnect
                gap_advertisements_enable(false);
                pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;
            }
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            DEBUG_PRINT("HCI_EVENT_DISCONNECTION_COMPLETE\n");
            if (hci_event_disconnection_complete_get_connection_handle(packet) == le_con_handle) {
                le_con_handle = HCI_CON_HANDLE_INVALID;
                pybricks_con_handle = HCI_CON_HANDLE_INVALID;
                uart_con_handle = HCI_CON_HANDLE_INVALID;
            } else {
                for (uint8_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
                    pbdrv_bluetooth_peripheral_t *peri = pbdrv_bluetooth_peripheral_get_by_index(i);
                    if (peri && hci_event_disconnection_complete_get_connection_handle(packet) == peri->con_handle) {
                        DEBUG_PRINT("Peripheral %u with handle %u disconnected\n", i, peri->con_handle);
                        gatt_client_stop_listening_for_characteristic_value_updates(&peri->platform_state->notification);
                        peri->con_handle = HCI_CON_HANDLE_INVALID;
                    }
                }
            }
            break;

        case GAP_EVENT_ADVERTISING_REPORT: {
            uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(packet);
            uint8_t data_length = gap_event_advertising_report_get_data_length(packet);
            const uint8_t *data = gap_event_advertising_report_get_data(packet);

            if (pbdrv_bluetooth_observe_callback) {
                int8_t rssi = gap_event_advertising_report_get_rssi(packet);
                pbdrv_bluetooth_observe_callback(event_type, data, data_length, rssi);
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

    propagate_event(packet);
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
        case SM_EVENT_PAIRING_COMPLETE:
            DEBUG_PRINT("Pairing complete\n");
            break;
        case SM_EVENT_REENCRYPTION_STARTED: {
            bd_addr_t addr;
            sm_event_reencryption_complete_get_address(packet, addr);
            DEBUG_PRINT("Bonding information exists for addr type %u, identity addr %s -> start re-encryption\n",
                sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            break;
        }
        case SM_EVENT_REENCRYPTION_COMPLETE:
            DEBUG_PRINT("Re-encryption complete.\n");
            break;
        default:
            break;
    }

    propagate_event(packet);
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
    #if !PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE_SERVER
    return PBIO_ERROR_NOT_SUPPORTED;
    #endif

    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    init_advertising_data();
    gap_advertisements_enable(true);

    PBIO_OS_AWAIT_UNTIL(state, event_packet && HCI_EVENT_IS_COMMAND_COMPLETE(event_packet, hci_le_set_advertise_enable));

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
    #if !PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE_SERVER
    return PBIO_ERROR_NOT_SUPPORTED;
    #endif

    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

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

pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbdrv_bluetooth_peripheral_t *peri = context;
    pbdrv_bluetooth_ad_match_result_flags_t flags;
    uint8_t btstack_error;

    // Operation can be explicitly cancelled or automatically on inactivity.
    if (!peri->cancel) {
        peri->cancel = pbio_os_timer_is_expired(&peri->watchdog);
    }

    PBIO_OS_ASYNC_BEGIN(state);

    memset(peri->platform_state, 0, sizeof(pbdrv_bluetooth_peripheral_platform_state_t));

    peri->con_handle = HCI_CON_HANDLE_INVALID;

    // active scanning to get scan response data.
    // scan interval: 48 * 0.625ms = 30ms
    gap_set_scan_params(1, 0x30, 0x30, 0);
    gap_start_scan();

start_scan:

    // Wait for advertisement that matches the filter unless timed out or cancelled.
    PBIO_OS_AWAIT_UNTIL(state, (peri->config->timeout && pbio_os_timer_is_expired(&peri->timer)) ||
        peri->cancel || (hci_event_is_type(event_packet, GAP_EVENT_ADVERTISING_REPORT) && ({

        uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(event_packet);
        const uint8_t *data = gap_event_advertising_report_get_data(event_packet);
        bd_addr_t address;
        gap_event_advertising_report_get_address(event_packet, address);

        // Match advertisement data against context-specific filter.
        flags = peri->config->match_adv(peri->user, event_type, data, NULL, address, peri->bdaddr);

        // Store the address to compare with scan response later.
        if (flags & PBDRV_BLUETOOTH_AD_MATCH_VALUE) {
            memcpy(peri->bdaddr, address, sizeof(bd_addr_t));
            peri->bdaddr_type = gap_event_advertising_report_get_address_type(event_packet);
        }

        // Wait condition: Advertisement matched and it isn't the same as before.
        // If it was the same and we're here, it means the scan response didn't match
        // so we shouldn't try it again.
        (flags & PBDRV_BLUETOOTH_AD_MATCH_VALUE) && !(flags & PBDRV_BLUETOOTH_AD_MATCH_ADDRESS);
    })));

    if ((peri->config->timeout && pbio_os_timer_is_expired(&peri->timer)) || peri->cancel) {
        DEBUG_PRINT("Scan %s.\n", peri->cancel ? "canceled": "timed out");
        gap_stop_scan();
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    DEBUG_PRINT("Advertisement matched, waiting for scan response\n");

    // The user timeout applies only to finding the device. We still want to
    // have a reasonable timeout for the scan response, connecting and pairing.
    pbio_os_timer_set(&peri->timer, PERIPHERAL_TIMEOUT_MS_SCAN_RESPONSE);

    // Wait for advertising response that matches the filter unless timed out or cancelled.
    PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&peri->timer) || peri->cancel ||
        (hci_event_is_type(event_packet, GAP_EVENT_ADVERTISING_REPORT) && ({

        uint8_t event_type = gap_event_advertising_report_get_advertising_event_type(event_packet);
        const uint8_t *data = gap_event_advertising_report_get_data(event_packet);
        char *detected_name = (char *)&data[2];
        bd_addr_t address;
        gap_event_advertising_report_get_address(event_packet, address);

        flags = peri->config->match_adv_rsp(peri->user, event_type, NULL, detected_name, address, peri->bdaddr);

        (flags & PBDRV_BLUETOOTH_AD_MATCH_VALUE) && (flags & PBDRV_BLUETOOTH_AD_MATCH_ADDRESS);
    })));

    if (pbio_os_timer_is_expired(&peri->timer) || peri->cancel) {
        DEBUG_PRINT("Scan response %s.\n", peri->cancel ? "canceled": "timed out");
        gap_stop_scan();
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    if (flags & PBDRV_BLUETOOTH_AD_MATCH_NAME_FAILED) {
        DEBUG_PRINT("Name requested but did not match. Scan again.\n");
        goto start_scan;
    }

    // When we get here, we have just matched a scan response and we are still
    // handling the same event packet, so we can still extract the name.
    const uint8_t *data = gap_event_advertising_report_get_data(event_packet);
    if (data[1] == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME) {
        memcpy(peri->name, &data[2], sizeof(peri->name));
    }

    DEBUG_PRINT("Scan response matched, initiate connection to %s.\n", bd_addr_to_str(peri->bdaddr));

    // We can stop scanning now.
    gap_stop_scan();

    // Initiate connection and await connection complete event.
    pbio_os_timer_set(&peri->timer, PERIPHERAL_TIMEOUT_MS_CONNECT);
    btstack_error = gap_connect(peri->bdaddr, peri->bdaddr_type);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }
    PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&peri->timer) || peri->cancel ||
        hci_event_le_peripheral_did_connect(event_packet));

    // If we timed out or were cancelled, abort the connection. We have to check
    // the event again in case cancellation and connection completed simultaneously.
    if (!hci_event_le_peripheral_did_connect(event_packet)) {
        DEBUG_PRINT("Connection %s.\n", peri->cancel ? "canceled": "timed out");
        gap_connect_cancel();
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    // The wait above was not interrupted, so we are now connected and the event
    // packet is still valid.
    peri->con_handle = hci_subevent_le_connection_complete_get_connection_handle(event_packet);
    DEBUG_PRINT("Connected with handle %d.\n", peri->con_handle);

    // We are done if no pairing is requested.
    if (!peri->config->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR) {
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
    sm_request_pairing(peri->con_handle);

    // Wait for pairing to complete unless timed out, cancelled, or disconnected.
    PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&peri->timer) || peri->cancel ||
        !pbdrv_bluetooth_peripheral_is_connected(peri) ||
        hci_event_le_peripheral_pairing_did_complete(event_packet, peri->con_handle));

    // If we timed out or were cancelled, disconnect and leave.
    if (!hci_event_le_peripheral_pairing_did_complete(event_packet, peri->con_handle)) {
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
    if (hci_event_is_type(event_packet, SM_EVENT_REENCRYPTION_COMPLETE)) {
        btstack_error = sm_event_reencryption_complete_get_status(event_packet);
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
        PBIO_OS_AWAIT_ONCE(state);
        goto start_pairing;

    } else if (hci_event_is_type(event_packet, SM_EVENT_PAIRING_COMPLETE)) {
        // Otherwise we have received pairing complete. Check status and disconnect
        // on failure.
        btstack_error = sm_event_pairing_complete_get_status(event_packet);
        DEBUG_PRINT("New pairing completed with error %u and reason %u.\n",
            btstack_error, sm_event_pairing_complete_get_reason(event_packet));

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
    }

    // Unreachable. We should have hit either of the above events.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbdrv_bluetooth_peripheral_t *peri = context;
    gatt_client_characteristic_t *current_char = &peri->platform_state->current_char;

    uint8_t btstack_error;

    PBIO_OS_ASYNC_BEGIN(state);

    // Nothing found to begin with.
    peri->char_disc.handle = 0;

    uint16_t handle_max = peri->char_disc.handle_max ? peri->char_disc.handle_max : 0xffff;

    // Start characteristic discovery.
    btstack_error = peri->char_disc.uuid16 ?
        gatt_client_discover_characteristics_for_handle_range_by_uuid16(
        packet_handler, peri->con_handle, 0x0001, handle_max, peri->char_disc.uuid16) :
        gatt_client_discover_characteristics_for_handle_range_by_uuid128(
        packet_handler, peri->con_handle, 0x0001, handle_max, peri->char_disc.uuid128);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    DEBUG_PRINT("Discovering characteristic\n");
    // Await until discovery is complete, processing each discovered
    // characteristic along the way to see if it matches what we need.
    PBIO_OS_AWAIT_UNTIL(state, ({

        if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
            // Got disconnected while waiting.
            return PBIO_ERROR_NO_DEV;
        }

        // Process a discovered characteristic, saving only the first match.
        if (!peri->char_disc.handle &&
            hci_event_is_type(event_packet, GATT_EVENT_CHARACTERISTIC_QUERY_RESULT) &&
            gatt_event_characteristic_query_result_get_handle(event_packet) == peri->con_handle) {

            // Unpack the result.
            gatt_event_characteristic_query_result_get_characteristic(event_packet, current_char);

            // We only care about the one characteristic that has at least the requested properties.
            if ((current_char->properties & peri->char_disc.properties) == peri->char_disc.properties) {
                DEBUG_PRINT("Found characteristic handle: 0x%04x with properties 0x%04x \n", current_char->value_handle, current_char->properties);
                peri->char_disc.handle = current_char->value_handle;
                peri->char_disc.handle_max = current_char->end_handle;
            }
        }

        // The wait until condition: discovery complete.
        hci_event_is_type(event_packet, GATT_EVENT_QUERY_COMPLETE) &&
        gatt_event_query_complete_get_handle(event_packet) == peri->con_handle;
    }));

    // Result of discovery.
    btstack_error = gatt_event_query_complete_get_att_status(event_packet);
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
    btstack_error = gatt_client_write_client_characteristic_configuration(
        packet_handler, peri->con_handle, current_char, GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);

    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    // We will be waiting for another GATT_EVENT_QUERY_COMPLETE, but the
    // current event is exactly this, so yield and wait for the next one.
    PBIO_OS_AWAIT_ONCE(state);

    DEBUG_PRINT("Waiting for notifications to be enabled.\n");
    PBIO_OS_AWAIT_UNTIL(state, ({
        if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
            // Got disconnected while waiting.
            return PBIO_ERROR_NO_DEV;
        }
        hci_event_is_type(event_packet, GATT_EVENT_QUERY_COMPLETE) &&
        gatt_event_query_complete_get_handle(event_packet) == peri->con_handle;
    }));

    // Result of enabling notifications.
    btstack_error = gatt_event_query_complete_get_att_status(event_packet);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    // Register notification handler.
    gatt_client_listen_for_characteristic_value_updates(
        &peri->platform_state->notification, packet_handler, peri->con_handle, current_char);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbdrv_bluetooth_peripheral_t *peri = context;

    uint8_t btstack_error;

    PBIO_OS_ASYNC_BEGIN(state);

    if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
        return PBIO_ERROR_NO_DEV;
    }

    gatt_client_characteristic_t characteristic = {
        .value_handle = peri->char_handle,
    };
    btstack_error = gatt_client_read_value_of_characteristic(packet_handler, peri->con_handle, &characteristic);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    // Await until read is complete, processing the result along the way.
    PBIO_OS_AWAIT_UNTIL(state, ({
        if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
            // Got disconnected while waiting.
            return PBIO_ERROR_NO_DEV;
        }
        // Cache the result.
        if (hci_event_is_type(event_packet, GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT) &&
            gatt_event_characteristic_value_query_result_get_handle(event_packet) == peri->con_handle &&
            gatt_event_characteristic_value_query_result_get_value_handle(event_packet) == peri->char_handle
            ) {
            peri->char_size = gatt_event_characteristic_value_query_result_get_value_length(event_packet);
            memcpy(peri->char_data, gatt_event_characteristic_value_query_result_get_value(event_packet), peri->char_size);
        }

        // The wait until condition: read complete.
        hci_event_is_type(event_packet, GATT_EVENT_QUERY_COMPLETE) &&
        gatt_event_query_complete_get_handle(event_packet) == peri->con_handle;
    }));

    // Result of read operation.
    btstack_error = gatt_event_query_complete_get_att_status(event_packet);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbdrv_bluetooth_peripheral_t *peri = context;

    uint8_t btstack_error;

    PBIO_OS_ASYNC_BEGIN(state);

    btstack_error = gatt_client_write_value_of_characteristic(packet_handler,
        peri->con_handle, peri->char_handle, peri->char_size, peri->char_data);

    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    PBIO_OS_AWAIT_UNTIL(state, ({
        if (!pbdrv_bluetooth_peripheral_is_connected(peri)) {
            // Got disconnected while waiting.
            return PBIO_ERROR_NO_DEV;
        }

        // The wait until condition: write complete.
        hci_event_is_type(event_packet, GATT_EVENT_QUERY_COMPLETE) &&
        gatt_event_query_complete_get_handle(event_packet) == peri->con_handle;
    }));

    // Result of write operation.
    btstack_error = gatt_event_query_complete_get_att_status(event_packet);
    if (btstack_error != ERROR_CODE_SUCCESS) {
        return att_error_to_pbio_error(btstack_error);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context) {
    if (!pbdrv_bluetooth_btstack_ble_supported()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    pbdrv_bluetooth_peripheral_t *peri = context;

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

static int link_db_entry_size() {
    return sizeof(bd_addr_t) + sizeof(link_key_t) + sizeof(link_key_type_t);
}

static int link_db_overhead() {
    return 2;  // 1 byte for length, 1 byte for checksum.
}

static int link_db_max_entries() {
    const int result = (PBSYS_CONFIG_BLUETOOTH_CLASSIC_LINK_KEY_DB_SIZE - link_db_overhead()) / link_db_entry_size();
    if (result > 255) {
        return 255;  // Only 1 byte to store the count.
    }
    return result;
}

// Computes the checksum for the link db, which we use to verify data integrity.
static uint8_t link_db_compute_checksum(uint8_t *data) {
    uint16_t count = data[0];
    const int end_offset = link_db_overhead() + count * link_db_entry_size();
    uint8_t checksum = 0;
    for (int off = 0; off < end_offset; off++) {
        checksum ^= data[off];
    }
    return checksum;
}

static uint8_t link_db_read_checksum(uint8_t *data) {
    uint16_t count = data[0];
    const int end_offset = link_db_overhead() + count * link_db_entry_size();
    return data[end_offset];
}

// Stores the link db to the settings and requests that it be written to flash.
static void link_db_settings_save(void) {
    btstack_link_key_iterator_t it;
    if (!gap_link_key_iterator_init(&it)) {
        DEBUG_PRINT("Failed to initialize link key iterator\n");
        return;
    }
    uint8_t *base = pbsys_storage_settings_get_link_key_db();
    if (base == NULL) {
        DEBUG_PRINT("No storage for link key database in settings\n");
        gap_link_key_iterator_done(&it);
        return;
    }
    uint16_t off = 0;

    uint8_t count = 0;
    off += 1;  // The first entry starts at offset 1.

    bdaddr_t addr;
    link_key_t key;
    link_key_type_t type;
    while (count < link_db_max_entries() &&
           gap_link_key_iterator_get_next(&it, addr, key, &type)) {
        memcpy(base + off, addr, sizeof(bd_addr_t));
        off += sizeof(bd_addr_t);
        memcpy(base + off, key, sizeof(link_key_t));
        off += sizeof(link_key_t);
        base[off] = type;
        off += sizeof(link_key_type_t);
        count++;
    }
    gap_link_key_iterator_done(&it);
    base[0] = count;
    base[off] = link_db_compute_checksum(base);
    pbsys_storage_request_write();
    DEBUG_PRINT("Saved %u link keys to settings\n", count);
}

// Loads the link DB from the settings.
// Note that the link db cannot be loaded until after the storage settings
// are ready. This might happen after bluetooth initialization, so we first
// load them instead when we become connectable (we won't need any link key
// information while we're not connectable anyway, since all connection
// requests will be rejected). We become connectable 1. during listen() and
// 2. during connect().
static void link_db_settings_load_once(void) {
    static bool loaded = false;
    if (loaded || !pbsys_storage_settings_ready()) {
        return;
    }
    loaded = true;

    uint8_t *base = pbsys_storage_settings_get_link_key_db();
    if (base == NULL) {
        DEBUG_PRINT("No link key database in settings\n");
        return;
    }
    uint16_t count = base[0];
    if (count > link_db_max_entries()) {
        DEBUG_PRINT("Link key database has invalid entry count, ignoring: %u\n",
            count);
        return;
    }

    if (link_db_read_checksum(base) != link_db_compute_checksum(base)) {
        DEBUG_PRINT("Link key database has invalid checksum, ignoring.\n");
        return;
    }

    uint16_t off = 1;
    for (uint16_t i = 0; i < count; i++) {
        bdaddr_t addr;
        link_key_t key;
        link_key_type_t type;
        memcpy(addr, base + off, sizeof(bd_addr_t));
        off += sizeof(bd_addr_t);
        memcpy(key, base + off, sizeof(link_key_t));
        off += sizeof(link_key_t);
        type = base[off];
        off += sizeof(link_key_type_t);
        gap_store_link_key_for_bd_addr(addr, key, type);
    }
    DEBUG_PRINT("Loaded %u link keys from settings\n", count);
}

// Returns true if the given address is already paired in our link database.
bool is_already_paired(bd_addr_t addr) {
    link_key_t key;
    link_key_type_t type;
    return gap_get_link_key_for_bd_addr(addr, key, &type);
}

#ifndef MAX_NR_RFCOMM_CHANNELS
#define MAX_NR_RFCOMM_CHANNELS 0
#endif

#define RFCOMM_SOCKET_COUNT (MAX_NR_RFCOMM_CHANNELS > 0 ? MAX_NR_RFCOMM_CHANNELS - 1 : 0)
#if HAVE_UMM_MALLOC
#define RFCOMM_RX_BUFFER_SIZE (4 * 1024)
#define RFCOMM_TX_BUFFER_SIZE (2 * 1024)
#else
// When we don't have umm_malloc, statically allocate small buffers.
// This limits throughput but doesn't constraint the logical size of
// messages we can send.
#define RFCOMM_RX_BUFFER_SIZE (256)
#define RFCOMM_TX_BUFFER_SIZE (256)
#endif

typedef struct {
    #if HAVE_UMM_MALLOC
    uint8_t *tx_buffer_data; // tx_buffer from customer. We don't own this.
    uint8_t *rx_buffer_data;
    #else
    uint8_t tx_buffer_data[RFCOMM_TX_BUFFER_SIZE];
    uint8_t rx_buffer_data[RFCOMM_RX_BUFFER_SIZE];
    #endif
    pbio_os_timer_t tx_timer; // Timer for tracking timeouts on the current send.
    pbio_os_timer_t rx_timer; // Timer for tracking timeouts on the current receive.
    lwrb_t tx_buffer;         // Ring buffer to contain outgoing data.
    lwrb_t rx_buffer;         // Ring buffer to contain incoming data.

    int mtu; // MTU for this connection.

    // How many rfcomm credits are outstanding? When the connection is first started,
    // this is the rx buffer size divided by the MTU (the frame size). Each time we receive
    // a frame, this decreases by one. When frames are consumed by a reader, or if
    // the discrepancy between what we can hold and what is outstanding grows
    // too large, we grant more credits.
    int credits_outstanding;

    pbio_error_t err;         // The first encountered error.
    uint16_t cid;             // The local rfcomm connection handle.
    uint16_t server_channel;  // The remote rfcomm channel we're connected to.
    bool is_used;             // Is this socket descriptor in use?
    bool is_connected;        // Is this socket descriptor connected?
    bool is_cancelled;        // Has this socket been cancelled? Interrupts pending
                              // listen() or connect calls.
    bool is_using_sdp_system; // Is this socket currently using the SDP system?
} pbdrv_bluetooth_classic_rfcomm_socket_t;

static pbdrv_bluetooth_classic_rfcomm_socket_t pbdrv_bluetooth_classic_rfcomm_sockets[RFCOMM_SOCKET_COUNT];

static int pbdrv_bluetooth_classic_rfcomm_socket_max_credits(pbdrv_bluetooth_classic_rfcomm_socket_t *socket) {
    return RFCOMM_RX_BUFFER_SIZE / socket->mtu;
}

// Give back credits that we owe and which we have space available to serve.
// We will give back credits when:
// 1. We owe two or more.
// 2. We owe one and the peer has no credits available.
// In no event will we give back credits such that the peer has enough credits
// to overflow our rx buffer.
static void pbdrv_bluetooth_classic_rfcomm_socket_grant_owed_credits(pbdrv_bluetooth_classic_rfcomm_socket_t *socket) {
    const int avail_frames = lwrb_get_free(&socket->rx_buffer) / socket->mtu;
    const int max_credits = pbdrv_bluetooth_classic_rfcomm_socket_max_credits(socket);
    int max_grant = max_credits - socket->credits_outstanding;
    if (max_grant > avail_frames) {
        max_grant = avail_frames;
    }
    if (max_grant > 1 || (max_grant > 0 && socket->credits_outstanding <= 1)) {
        rfcomm_grant_credits(socket->cid, max_grant);
        socket->credits_outstanding += max_grant;
    }
}

static void pbdrv_bluetooth_classic_rfcomm_socket_reset(pbdrv_bluetooth_classic_rfcomm_socket_t *socket) {
    #if HAVE_UMM_MALLOC
    if (socket->rx_buffer_data) {
        umm_free(socket->rx_buffer_data);
        socket->rx_buffer_data = NULL;
    }
    if (socket->tx_buffer_data) {
        umm_free(socket->tx_buffer_data);
        socket->tx_buffer_data = NULL;
    }
    #endif
    lwrb_free(&socket->rx_buffer);
    lwrb_free(&socket->tx_buffer);
    // A non-zero duration on these timers is used as a flag to indicate the
    // presence of a deadline on a connection attempt.
    socket->rx_timer.duration = 0;
    socket->tx_timer.duration = 0;
    socket->is_used = false;
    socket->is_connected = false;
    socket->is_cancelled = false;
    socket->is_using_sdp_system = false;
    socket->cid = (uint16_t)-1;
    socket->mtu = 0;
    socket->credits_outstanding = 0;
    socket->err = PBIO_SUCCESS;
}

static pbdrv_bluetooth_classic_rfcomm_socket_t *pbdrv_bluetooth_classic_rfcomm_socket_alloc() {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        if (!pbdrv_bluetooth_classic_rfcomm_sockets[i].is_used) {
            pbdrv_bluetooth_classic_rfcomm_socket_t *sock = &pbdrv_bluetooth_classic_rfcomm_sockets[i];
            pbdrv_bluetooth_classic_rfcomm_socket_reset(sock);
            sock->is_used = true;
            #if HAVE_UMM_MALLOC
            sock->rx_buffer_data = umm_malloc(RFCOMM_RX_BUFFER_SIZE);
            sock->tx_buffer_data = umm_malloc(RFCOMM_TX_BUFFER_SIZE);
            if (!sock->rx_buffer_data || !sock->tx_buffer_data) {
                DEBUG_PRINT("Failed to allocate RFCOMM RX or TX buffer.\n");
                sock->is_used = false;
                return NULL;
            }
            #endif
            lwrb_init(&sock->rx_buffer, sock->rx_buffer_data, RFCOMM_RX_BUFFER_SIZE);
            lwrb_init(&sock->tx_buffer, sock->tx_buffer_data, RFCOMM_TX_BUFFER_SIZE);
            return sock;
        }
    }
    DEBUG_PRINT("[btc] Alloc failed; all sockets in use.\n");
    return NULL;
}


static pbdrv_bluetooth_classic_rfcomm_socket_t *pbdrv_bluetooth_classic_rfcomm_socket_find_by_cid(uint16_t cid) {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        if (pbdrv_bluetooth_classic_rfcomm_sockets[i].is_used &&
            pbdrv_bluetooth_classic_rfcomm_sockets[i].cid == cid) {
            return &pbdrv_bluetooth_classic_rfcomm_sockets[i];
        }
    }
    return NULL;
}

static pbdrv_bluetooth_classic_rfcomm_socket_t *pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(const pbdrv_bluetooth_rfcomm_conn_t *c) {
    if (c->conn_id < 0 || c->conn_id >= RFCOMM_SOCKET_COUNT) {
        return NULL;
    }
    return &pbdrv_bluetooth_classic_rfcomm_sockets[c->conn_id];
}

static int pbdrv_bluetooth_classic_rfcomm_socket_id(pbdrv_bluetooth_classic_rfcomm_socket_t *socket) {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        if (&pbdrv_bluetooth_classic_rfcomm_sockets[i] == socket) {
            return i;
        }
    }
    return -1;
}

static pbdrv_bluetooth_classic_rfcomm_socket_t *pending_listen_socket;

void user_rfcomm_event_handler(uint8_t *packet, uint16_t size) {
    uint8_t event_type = hci_event_packet_get_type(packet);
    switch (event_type) {
        case RFCOMM_EVENT_CHANNEL_OPENED: {
            // TODO: rescue the linked key for this address if we're above capacity in the saved settings buffer.
            uint8_t bdaddr[6];
            rfcomm_event_channel_opened_get_bd_addr(packet, bdaddr);
            uint16_t cid = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
            pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_cid(cid);

            if (!sock) {
                // If we have no allocated socket for this cid, disconnect. It shouldn't
                // be possible for this to happen, but just in case . . .
                DEBUG_PRINT("Unknown cid (%u) associated with address %s\n",
                    cid, bd_addr_to_str(bdaddr));
                rfcomm_disconnect(cid);
                break;
            }
            int status = rfcomm_event_channel_opened_get_status(packet);
            if (status != 0) {
                DEBUG_PRINT("RFCOMM channel open failed with status: %d", status);
                sock->err = PBIO_ERROR_FAILED;
            } else {
                DEBUG_PRINT("RFCOMM channel opened: cid=%u.\n", cid);
                sock->mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
                if (sock->mtu == 0) {
                    rfcomm_disconnect(cid);
                    DEBUG_PRINT("RFCOMM channel opened with invalid MTU=0, dropping connection.\n");
                    sock->err = PBIO_ERROR_FAILED;
                }
                sock->is_connected = true;
                sock->credits_outstanding = 0;
                pbdrv_bluetooth_classic_rfcomm_socket_grant_owed_credits(sock);
            }
            break;
        }

        case RFCOMM_EVENT_INCOMING_CONNECTION: {
            uint16_t cid = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);

            if (!pending_listen_socket) {
                DEBUG_PRINT("Received unexpected incoming RFCOMM connection.\n");
                rfcomm_disconnect(cid);
                break;
            }
            pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pending_listen_socket;
            pending_listen_socket = NULL;

            // Note: we aren't connected yet. We'll get an RFCOMM_EVENT_CHANNEL_OPENED for
            // this channel once we're officially online.
            rfcomm_accept_connection(cid);
            sock->cid = cid;

            // Mark ourselves as no longer connectable, since we aren't listening.
            gap_connectable_control(0);
            break;
        }

        case RFCOMM_EVENT_CAN_SEND_NOW: {
            uint16_t cid = rfcomm_event_can_send_now_get_rfcomm_cid(packet);
            pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_cid(cid);
            if (!sock) {
                DEBUG_PRINT("Unknown cid (%u) for CAN_SEND_NOW event, dropping connection.\n", cid);
                rfcomm_disconnect(cid);
                break;
            }

            if (lwrb_get_full(&sock->tx_buffer) == 0) {
                // Nothing to send. This is normal, since we get this event immediately when the
                // channel is opened, and the Python code won't usually have gotten around to giving
                // us a transmit buffer yet.
                break;
            }

            uint8_t *data = lwrb_get_linear_block_read_address(&sock->tx_buffer);
            uint16_t write_len = lwrb_get_linear_block_read_length(&sock->tx_buffer);


            if (write_len > sock->mtu) {
                write_len = sock->mtu;
            }

            int err = rfcomm_send(sock->cid, data, write_len);
            lwrb_skip(&sock->tx_buffer, write_len);
            if (err) {
                DEBUG_PRINT("Failed to send RFCOMM data: %d\n", err);
                sock->err = PBIO_ERROR_FAILED;
                rfcomm_disconnect(sock->cid);
                break;
            }


            if (lwrb_get_full(&sock->tx_buffer) == 0) {
                pbio_os_request_poll();
            } else {
                // If there's more data we need to do another send request.
                rfcomm_request_can_send_now_event(sock->cid);
            }

            break;
        }

        case RFCOMM_EVENT_CHANNEL_CLOSED: {
            uint16_t cid = rfcomm_event_channel_closed_get_rfcomm_cid(packet);
            pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_cid(cid);
            if (!sock) {
                // If !sock, we closed the channel ourselves and there's nothing left to do.
                break;
            }

            DEBUG_PRINT("RFCOMM_EVENT_CHANNEL_CLOSED by remote for cid=%u.\n", cid);

            // Note: we do not reset the socket, since the user is expected
            // to call pbdrv_bluetooth_rfcomm_close() or disconnect_all() first.
            if (sock->is_connected) {
                DEBUG_PRINT("RFCOMM channel closed: cid=%u.\n", cid);
                sock->err = PBIO_ERROR_IO;
                sock->is_connected = false;
            }
            break;
        }

        default:
            DEBUG_PRINT("Received unknown RFCOMM event: %u\n", event_type);
            break;
    }
}

void user_rfcomm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    switch (packet_type) {
        case HCI_EVENT_PACKET: {
            user_rfcomm_event_handler(packet, size);
            break;
        }
        case RFCOMM_DATA_PACKET: {
            pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_cid(channel);

            if (!sock) {
                DEBUG_PRINT("Received RFCOMM data for unknown channel: 0x%04x\n", channel);
                break;
            }

            if (size > lwrb_get_free(&sock->rx_buffer)) {
                DEBUG_PRINT("Received RFCOMM data that exceeds buffer capacity: %u\n", size);
                sock->err = PBIO_ERROR_FAILED;
            }

            lwrb_write(&sock->rx_buffer, packet, size);

            // Each packet we receive consumed a credit on the remote side.
            --sock->credits_outstanding;

            // See if we have enough space such that we should grand more credits.
            pbdrv_bluetooth_classic_rfcomm_socket_grant_owed_credits(sock);
            pbio_os_request_poll();
            break;
        }
    }
}

static bool hci_handle_to_bd_addr(uint16_t handle, bd_addr_t addr) {
    hci_connection_t *conn = hci_connection_for_handle(handle);
    if (!conn) {
        return false;
    }
    memcpy(addr, conn->address, sizeof(bd_addr_t));
    return true;
}

static void maybe_handle_classic_security_packet(uint8_t *packet, uint16_t size) {
    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
            // Pairing handlers, in case auto-accept doesn't work.
            bd_addr_t requester_addr;
            hci_event_user_confirmation_request_get_bd_addr(packet, requester_addr);
            DEBUG_PRINT("SSP User Confirmation Request. Auto-accepting...\n");
            gap_ssp_confirmation_response(requester_addr);
            break;
        }
        case HCI_EVENT_AUTHENTICATION_COMPLETE: {
            // If authentication fails, we may need to drop the link key from the database.
            uint8_t auth_status = hci_event_authentication_complete_get_status(packet);
            if (auth_status == ERROR_CODE_AUTHENTICATION_FAILURE || auth_status == ERROR_CODE_PIN_OR_KEY_MISSING) {
                DEBUG_PRINT("AUTH FAIL: Link key rejected/missing (Status 0x%02x).\n", auth_status);
                uint16_t handle = hci_event_authentication_complete_get_connection_handle(packet);
                bd_addr_t addr;
                if (!hci_handle_to_bd_addr(handle, addr)) {
                    DEBUG_PRINT("AUTH FAIL: Unknown address for handle 0x%04x\n", handle);
                    break;
                }
                gap_drop_link_key_for_bd_addr(addr);
                link_db_settings_save();
            }
            pbio_os_request_poll();
            break;
        }
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            // HCI disconnection events are our chance to check if we've failed to connect due to
            // some error in authentication, which might indicate that our link key database contains
            // some manner of stale key. This is not where we handle RFCOMM socket disconnections! See
            // below for the relevant RFCOMM events.
            uint8_t reason = hci_event_disconnection_complete_get_reason(packet);
            uint16_t handle = hci_event_disconnection_complete_get_connection_handle(packet);
            bd_addr_t addr;
            if (reason == ERROR_CODE_AUTHENTICATION_FAILURE) { // Authentication Failure
                if (!hci_handle_to_bd_addr(handle, addr)) {
                    // Can't do anything without the address.
                    DEBUG_PRINT("DISCONNECTED: Unknown address for handle 0x%04x\n", handle);
                    break;
                }
                DEBUG_PRINT("DISCONNECTED: Bad Link Key.\n");
                gap_drop_link_key_for_bd_addr(addr);
                link_db_settings_save();
            } else {
                DEBUG_PRINT("DISCONNECTED: Reason 0x%02x\n", reason);
            }

            break;
        }
        case HCI_EVENT_LINK_KEY_NOTIFICATION: {
            // BTStack has already updated the link key database, so we just need to save it.
            DEBUG_PRINT("Link key updated, saving to settings.\n");
            link_db_settings_save();
            break;
        }
    }
}

// Is some active rfcomm_connect call using the SDP system?
static bool sdp_system_in_use = false;
// Is there an ongoing SDP query? Even if nobody is using the SDP system (e.g. a
// query was started then abandoned), we can't issue a new query until we get
// the SDP_EVENT_QUERY_COMPLETE event.
static bool sdp_query_pending = false;
// Memory location for the result of the current SDP query.
static uint16_t sdp_query_rfcomm_channel;

static void bluetooth_btstack_classic_sdp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) {
        DEBUG_PRINT("SDP packet handler unexpected packet type %u\n",
            packet_type);
        return;
    }
    switch (hci_event_packet_get_type(packet)) {
        case SDP_EVENT_QUERY_RFCOMM_SERVICE: {
            if (!sdp_query_pending) {
                DEBUG_PRINT("Received unexpected SDP query result.\n");
                return;
            }
            if (sdp_query_rfcomm_channel != 1) {
                // Note: we prefer to return channel 1 over any other channel, since this is the default
                // spp profile channel. The main purpose of this SDP query is to find channels *other* than
                // one, since the default channel may not be served especially in the case of Windows bluetooth
                // com ports.
                //
                // One limitation of our implementation here is that we don't provide the user any way to select
                // between multiple RFCOMM channels. Perhaps in the future we will allow users to manually specify
                // the channel if they know their server will be listening on a channel other than 1. All EV3s
                // will listen on channel 1.
                sdp_query_rfcomm_channel =
                    sdp_event_query_rfcomm_service_get_rfcomm_channel(packet);
            }
            DEBUG_PRINT("Found RFCOMM channel: %u\n", sdp_query_rfcomm_channel);
            break;
        }
        case SDP_EVENT_QUERY_COMPLETE: {
            DEBUG_PRINT("SDP query complete.\n");
            sdp_query_pending = false;
            pbio_os_request_poll();
            break;
        }
        default: {
            DEBUG_PRINT("Received ignored SDP event: %u\n",
                hci_event_packet_get_type(packet));
            break;
        }
    }
}

void pbdrv_bluetooth_classic_init() {
    l2cap_init();
    rfcomm_init();
    sdp_init();

    gap_ssp_set_enable(1);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gap_ssp_set_auto_accept(true);
    gap_set_class_of_device(0x000804);  // Toy : Robot

    hci_set_link_key_db(btstack_link_key_db_memory_instance());

    hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);
    sdp_client_init();
    rfcomm_set_required_security_level(LEVEL_2);

    static uint8_t spp_service_buffer[150];
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));

    spp_create_sdp_record(spp_service_buffer, 0x10001, 1, "Pybricks RFCOMM Service");
    sdp_register_service(spp_service_buffer);

    // All EV3s listen only on channel 1 (the default SPP channel).
    rfcomm_register_service_with_initial_credits(&user_rfcomm_packet_handler, 1, 1024, 0);
}

void pbdrv_bluetooth_local_address(bdaddr_t addr) {
    gap_local_bd_addr(addr);
}

const char *pbdrv_bluetooth_bdaddr_to_str(const bdaddr_t addr) {
    return bd_addr_to_str(addr);
}

bool pbdrv_bluetooth_str_to_bdaddr(const char *str, bdaddr_t addr) {
    return sscanf_bd_addr(str, addr) == 1;
}

// Returns whether this socket is in a state where we should abandon
// our connection attempt, either listening or connecting.
static bool
should_abandon_connection(pbdrv_bluetooth_classic_rfcomm_socket_t *sock) {
    if (!sock) {
        return false;
    }
    if (sock->rx_timer.duration > 0 &&
        pbio_os_timer_is_expired(&sock->rx_timer)) {
        sock->err = PBIO_ERROR_TIMEDOUT;
        return true;
    }
    if (sock->is_cancelled) {
        sock->err = PBIO_ERROR_CANCELED;
        return true;
    }
    if (sock->err != PBIO_SUCCESS) {
        return true;
    }
    return false;
}

pbio_error_t
pbdrv_bluetooth_rfcomm_connect(pbio_os_state_t *state, bdaddr_t bdaddr,
    int32_t timeout,
    pbdrv_bluetooth_rfcomm_conn_t *conn) {

    // Each time we resume this function, we need to load the socket pointer.
    // On the first time through, initialize the conn_id to some non-matching
    // value to prevent finding a socket owned by someone else.
    if (*state == 0) {
        conn->conn_id = -1;
    }
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock =
        pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);
    if (should_abandon_connection(sock)) {
        goto cleanup;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    // The link db is loaded lazily since the storage settings system may not be
    // ready when the bluetooth system is initialized, but it will be ready by
    // the time we try to make any rfcomm connections.
    link_db_settings_load_once();

    sock = pbdrv_bluetooth_classic_rfcomm_socket_alloc();
    if (!sock) {
        DEBUG_PRINT("[btc:rfcomm_connect] No more sockets.\n");
        // In this one case we need to return directly, because the cleanup
        // handler would try to access sock.
        return PBIO_ERROR_RESOURCE_EXHAUSTED;
    }
    conn->conn_id = pbdrv_bluetooth_classic_rfcomm_socket_id(sock);

    if (timeout > 0) {
        // The rx_timer is used to track connection timeouts both for
        // rfcomm_connect and ..._listen.
        pbio_os_timer_set(&sock->rx_timer, timeout);
    }

    // Wait until the Bluetooth controller is up.
    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == HCI_STATE_WORKING);

    // Wait until any other pending SDP query is done.
    PBIO_OS_AWAIT_UNTIL(state, !sdp_query_pending && !sdp_system_in_use);

    // TODO: allow manually specifying the channel if the user knows it already.
    DEBUG_PRINT("[btc:rfcomm_connect] Starting SDP query...\n");
    sdp_system_in_use = true;
    sdp_query_pending = true;
    sock->is_using_sdp_system = true;

    // Note: valid server channels from 1-30, so this should never be returned
    // by a real SDP response.
#define SERVER_CHANNEL_UNSET ((uint16_t)-1)

    sdp_query_rfcomm_channel = sock->server_channel = SERVER_CHANNEL_UNSET;
    uint8_t sdp_err = sdp_client_query_rfcomm_channel_and_name_for_service_class_uuid(
        bluetooth_btstack_classic_sdp_packet_handler, (uint8_t *)bdaddr, BLUETOOTH_SERVICE_CLASS_SERIAL_PORT);
    if (sdp_err != 0) {
        DEBUG_PRINT("[btc:rfcomm_connect] Failed to start SDP query: %d\n", sdp_err);
        // Since we won't get any SDP query events following this, we must
        // manually mark the query as no longer pending.
        sdp_query_pending = false;
        sock->err = PBIO_ERROR_FAILED;
        goto cleanup;
    }
    PBIO_OS_AWAIT_UNTIL(state, !sdp_query_pending);
    sock->server_channel = sdp_query_rfcomm_channel;

    // Allow other SDP queries to go ahead.
    sdp_system_in_use = sock->is_using_sdp_system = false;
    if (sock->server_channel == SERVER_CHANNEL_UNSET) {
        DEBUG_PRINT("[btc:rfcomm_connect] Failed to find RFCOMM channel for device.\n");
        sock->err = PBIO_ERROR_FAILED;
        goto cleanup;
    }
    DEBUG_PRINT("[btc:rfcomm_connect] Found RFCOMM channel %d for device.\n", sock->server_channel);

    // We establish the channel with no credits. Once we know the negotiated
    // MTU, we can calculate the number of credits we should grant.
    uint8_t rfcomm_err;
    if ((rfcomm_err = rfcomm_create_channel_with_initial_credits(&user_rfcomm_packet_handler, (uint8_t *)bdaddr, sock->server_channel, 0, &sock->cid)) != 0) {
        DEBUG_PRINT("[btc:rfcomm_connect] Failed to create RFCOMM channel: %d\n", rfcomm_err);
        (void)rfcomm_err;
        sock->err = PBIO_ERROR_FAILED;
        goto cleanup;
    }
    PBIO_OS_AWAIT_UNTIL(state, sock->is_connected);
    if (!sock->is_connected) {
        goto cleanup;
    }

    DEBUG_PRINT("[btc:rfcomm_connect] Connected (cid=%d remote=%s mtu=%d server_chan=%d)\n",
        sock->cid, bd_addr_to_str(bdaddr), sock->mtu, sock->server_channel);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);

cleanup:
    if (sock->is_using_sdp_system) {
        sdp_system_in_use = false;
        sock->is_using_sdp_system = false;
    }
    pbio_error_t err = sock->err;
    pbdrv_bluetooth_classic_rfcomm_socket_reset(sock);
    return err;
}

pbio_error_t pbdrv_bluetooth_rfcomm_listen(
    pbio_os_state_t *state,
    int32_t timeout,
    pbdrv_bluetooth_rfcomm_conn_t *conn) {
    if (*state == 0) {
        conn->conn_id = -1;
    }
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock =
        pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);
    if (should_abandon_connection(sock)) {
        goto cleanup;
    }

    PBIO_OS_ASYNC_BEGIN(state);

    // The link db is loaded lazily since the storage settings system may not be
    // ready when the bluetooth system is initialized, but it will be ready by
    // the time we try to make any rfcomm connections.
    link_db_settings_load_once();

    sock = pbdrv_bluetooth_classic_rfcomm_socket_alloc();
    if (!sock) {
        DEBUG_PRINT("[btc:rfcomm_listen] No more sockets.\n");
        return PBIO_ERROR_RESOURCE_EXHAUSTED;
    }
    conn->conn_id = pbdrv_bluetooth_classic_rfcomm_socket_id(sock);

    if (timeout > 0) {
        // We use the rx timer to track listen timeouts, since we don't have
        // any other need for it until the connection is established.
        pbio_os_timer_set(&sock->rx_timer, timeout);
    }

    PBIO_OS_AWAIT_UNTIL(state, hci_get_state() == HCI_STATE_WORKING);
    if (pending_listen_socket) {
        // Unlike with connect, where it's plausible for multiple async contexts
        // to be connecting to different devices, it's always going to be an
        // error to listen more than once at a time.
        DEBUG_PRINT("[btc:rfcomm_listen] Already listening.\n");
        sock->err = PBIO_ERROR_BUSY;
        goto cleanup;
    }

    // Wait until either we time out, there is an error, or the socket is
    // connected.
    pending_listen_socket = sock;
    gap_connectable_control(1);
    DEBUG_PRINT("[btc:rfcomm_listen] Listening for incoming RFCOMM connections...\n");
    PBIO_OS_AWAIT_UNTIL(state, sock->is_connected);
    pending_listen_socket = NULL;

    if (sock->err != PBIO_SUCCESS) {
        DEBUG_PRINT("[btc:rfcomm_listen] Other error.\n");
        goto cleanup;
    }

    DEBUG_PRINT("[btc:rfcomm_listen] Connected\n");

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);

cleanup:
    ;
    pbio_error_t err = sock->err;
    pbdrv_bluetooth_classic_rfcomm_socket_reset(sock);
    return err;
}

pbio_error_t pbdrv_bluetooth_rfcomm_close(
    pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);
    if (!sock) {
        DEBUG_PRINT("[btc:rfcomm_close] Invalid CID: %d\n", conn->conn_id);
        conn->conn_id = -1;
        return PBIO_ERROR_INVALID_OP;
    }
    rfcomm_disconnect(sock->cid);
    pbdrv_bluetooth_classic_rfcomm_socket_reset(sock);
    conn->conn_id = -1;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_rfcomm_send(
    const pbdrv_bluetooth_rfcomm_conn_t *conn,
    const uint8_t *data,
    size_t length,
    size_t *bytes_sent) {
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);
    if (!sock || !sock->is_connected) {
        DEBUG_PRINT("[btc:rfcomm_send] Socket is not connected or does not exist.\n");
        return PBIO_ERROR_FAILED;
    }

    DEBUG_PRINT("[btc:rfcomm_send] Sending '%.*s' to RFCOMM channel.\n", (int)length, data);

    bool was_idle = lwrb_get_full(&sock->tx_buffer) == 0;
    *bytes_sent = lwrb_write(&sock->tx_buffer, data, length);
    if (was_idle && *bytes_sent > 0) {
        // If we were idle before, we need to request a send event to kick
        // things off.
        rfcomm_request_can_send_now_event(sock->cid);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_rfcomm_recv(
    const pbdrv_bluetooth_rfcomm_conn_t *conn,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *bytes_received) {
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);

    if (!sock || !sock->is_connected) {
        DEBUG_PRINT("[btc:rfcomm_recv] Socket is not connected or does not exist.\n");
        return PBIO_ERROR_FAILED;
    }

    *bytes_received = lwrb_read(&sock->rx_buffer, buffer, buffer_size);
    if (*bytes_received > 0) {
        // After reading data, we may have freed up enough space to grant some
        // credits back to our peer.
        pbdrv_bluetooth_classic_rfcomm_socket_grant_owed_credits(sock);
    }

    return PBIO_SUCCESS;
}

bool pbdrv_bluetooth_rfcomm_is_writeable(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);
    if (!sock || !sock->is_connected) {
        return false;
    }
    return lwrb_get_free(&sock->tx_buffer) > 0;
}

bool pbdrv_bluetooth_rfcomm_is_readable(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);
    if (!sock) {
        return false;
    }
    return lwrb_get_full(&sock->rx_buffer) > 0;
}

bool pbdrv_bluetooth_rfcomm_is_connected(const pbdrv_bluetooth_rfcomm_conn_t *conn) {
    pbdrv_bluetooth_classic_rfcomm_socket_t *sock = pbdrv_bluetooth_classic_rfcomm_socket_find_by_conn(conn);
    if (!sock) {
        return false;
    }
    return sock->is_connected;
}

void pbdrv_bluetooth_rfcomm_cancel_connection() {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        pbdrv_bluetooth_classic_rfcomm_socket_t *sock =
            &pbdrv_bluetooth_classic_rfcomm_sockets[i];
        if (sock->is_used) {
            sock->is_cancelled = true;
        }
    }
}

void pbdrv_bluetooth_rfcomm_disconnect_all() {
    for (int i = 0; i < RFCOMM_SOCKET_COUNT; i++) {
        pbdrv_bluetooth_classic_rfcomm_socket_t *sock =
            &pbdrv_bluetooth_classic_rfcomm_sockets[i];
        if (sock->is_used && sock->is_connected) {
            rfcomm_disconnect(sock->cid);
            pbdrv_bluetooth_classic_rfcomm_socket_reset(sock);
        }
    }
    pending_listen_socket = NULL;
}

static void pbdrv_bluetooth_inquiry_unpack_scan_event(uint8_t *event_packet, pbdrv_bluetooth_inquiry_result_t *result) {

    gap_event_inquiry_result_get_bd_addr(event_packet, result->bdaddr);
    if (gap_event_inquiry_result_get_rssi_available(event_packet)) {
        result->rssi = gap_event_inquiry_result_get_rssi(event_packet);
    }

    if (gap_event_inquiry_result_get_name_available(event_packet)) {
        const uint8_t *name = gap_event_inquiry_result_get_name(event_packet);
        const size_t name_len = gap_event_inquiry_result_get_name_len(event_packet);
        snprintf(result->name, sizeof(result->name), "%.*s", (int)name_len, name);
    }

    result->class_of_device = gap_event_inquiry_result_get_class_of_device(event_packet);
}

pbio_error_t pbdrv_bluetooth_inquiry_scan_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_classic_task_context_t *task = context;

    if (!task->cancel) {
        task->cancel = pbio_os_timer_is_expired(&task->watchdog);
    }

    PBIO_OS_ASYNC_BEGIN(state);

    DEBUG_PRINT("Start inquiry scan.\n");

    gap_inquiry_start(task->inq_duration);

    // Wait until scan timeout or the number of devices are found.
    PBIO_OS_AWAIT_UNTIL(state, (*task->inq_count == *task->inq_count_max) || ({

        if (task->cancel || *task->inq_count_max == 0) {
            // Cancelled or the external data no longer available. Stop
            // scanning and don't write any more data.
            DEBUG_PRINT("Inquiry scan canceled.\n");
            gap_inquiry_stop();
            return PBIO_ERROR_CANCELED;
        }

        // Process a scan result.
        if (hci_event_is_type(event_packet, GAP_EVENT_INQUIRY_RESULT)) {
            DEBUG_PRINT("Received scan result.\n");
            pbdrv_bluetooth_inquiry_result_t *result = &task->inq_results[(*task->inq_count)++];
            pbdrv_bluetooth_inquiry_unpack_scan_event(event_packet, result);

        }

        // The wait until condition: inquiry complete.
        hci_event_is_type(event_packet, GAP_EVENT_INQUIRY_COMPLETE);
    }));

    DEBUG_PRINT("Inquiry scan ended with %d results.\n", *task->inq_count);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

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

    if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_HCI)) {
        return PBIO_ERROR_INVALID_OP;
    }

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
}

pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // Wait for power on.
    PBIO_OS_AWAIT(state, &sub, bluetooth_btstack_handle_power_control(&sub, HCI_POWER_ON, HCI_STATE_WORKING));

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

    // Also propagate non-btstack events like polls or timers.
    propagate_event(NULL);

    return bluetooth_thread_err;
}

// Have to be defined, but are not used.
const uint32_t cc256x_init_script_size = 0;
const uint8_t cc256x_init_script[] = {};

void pbdrv_bluetooth_init_hci(void) {

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
    hci_dump_enable_packet_log(false);
    #endif

    static btstack_packet_callback_registration_t hci_event_callback_registration;

    // Attach btstack platform state to peripherals.
    for (uint8_t i = 0; i < PBDRV_CONFIG_BLUETOOTH_NUM_PERIPHERALS; i++) {
        pbdrv_bluetooth_peripheral_t *peri = pbdrv_bluetooth_peripheral_get_by_index(i);
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
    static btstack_packet_callback_registration_t sm_event_callback_registration;
    sm_event_callback_registration.callback = &sm_packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    gap_random_address_set_mode(GAP_RANDOM_ADDRESS_NON_RESOLVABLE);
    gap_set_max_number_peripheral_connections(2);

    // GATT Client setup
    gatt_client_init();

    pbdrv_bluetooth_classic_init();

    #if PBDRV_CONFIG_BLUETOOTH_BTSTACK_LE_SERVER
    // setup ATT server
    att_server_init(profile_data, att_read_callback, NULL);

    device_information_service_server_init();
    device_information_service_server_set_firmware_revision(PBIO_VERSION_STR);
    device_information_service_server_set_software_revision(PBIO_PROTOCOL_VERSION_STR);
    device_information_service_server_set_pnp_id(0x01, LWP3_LEGO_COMPANY_ID, HUB_KIND, HUB_VARIANT);

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
