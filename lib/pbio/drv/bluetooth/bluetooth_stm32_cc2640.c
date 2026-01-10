// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// Bluetooth for STM32 MCU with TI CC2640

// Useful docs:
// - https://dev.ti.com/tirex/explore/content/simplelink_cc13x2_26x2_sdk_3_10_00_53/docs/ble5stack/ble_user_guide/html/ble-stack-common/npi-index.html#npi-handshake
// - http://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/538/3583.BLE-SPI-Driver-Design-External.pdf

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_STM32_CC2640

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/clock.h>
#include <pbdrv/gpio.h>
#include <pbdrv/random.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbio/version.h>
#include <pbsys/config.h>
#include <pbsys/storage.h>

#include <lego/lwp3.h>

#include <att.h>
#include <gap.h>
#include <gatt.h>
#include <gattservapp.h>
#include <hci_ext.h>
#include <hci_tl.h>
#include <hci.h>
#include <util.h>

#include "./bluetooth.h"
#include "./bluetooth_stm32_cc2640.h"

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DBG(...)
#define DEBUG_PRINT pbio_debug
#else
#define DBG(...)
#define DEBUG_PRINT(...)
#endif
// hub name goes in special section so that it can be modified when flashing firmware
__attribute__((section(".name")))
char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

static char pbdrv_bluetooth_fw_version[16]; // vX.XX.XX

// used to identify which hub - Device Information Service (DIS).
// 0x2A50 - service UUID - PnP ID characteristic UUID
// 0x01 - Vendor ID Source Field - Bluetooth SIG-assigned ID
// 0x0397 - Vendor ID Field - LEGO company identifier
// 0x00XX - Product ID Field - hub device ID
// 0x0000 - Product Version Field - not applicable to most hubs
#define PNP_ID "\x50\x2a\x01\x97\x03" PBDRV_CONFIG_BLUETOOTH_STM32_CC2640_HUB_ID "\x00\x00\x00"

#ifndef PBDRV_CONFIG_BLUETOOTH_STM32_CC2640_HUB_ID
#error "Must define PBDRV_CONFIG_BLUETOOTH_STM32_CC2640_HUB_ID"
#endif

// TI Network Processor Interface (NPI)
#define NPI_SPI_SOF             0xFE    // start of frame
#define NPI_SPI_HEADER_LEN      3       // zero pad, SOF, length

#define NO_CONNECTION           0xFFFF
#define NO_AUTH                 0xFF
#define GAP_BOND_ERR_OFFSET     0x600

typedef enum {
    RESET_STATE_OUT_HIGH,   // in reset
    RESET_STATE_OUT_LOW,    // out of reset
    RESET_STATE_INPUT,      // ?
} reset_state_t;

// Tx buffer for SPI writes
static uint8_t write_buf[TX_BUFFER_SIZE];
// Rx buffer for SPI reads
static uint8_t read_buf[RX_BUFFER_SIZE];

// size of current SPI xfer Tx data
// value is set to 0 when Tx is complete
static uint8_t write_xfer_size;

// reflects state of SRDY signal
volatile bool spi_srdy;
// count of SRDY signal falling edges
volatile uint8_t spi_n_srdy_count;
// set to false when xfer is started and true when xfer is complete
volatile bool spi_xfer_complete;

// set to the pending hci command opcode when a command is sent
static uint16_t hci_command_opcode;
// set to false when hci command is started and true when command status is received
static bool hci_command_status;
// set to false when hci command is started and true when command is completed
static bool hci_command_complete;
// used to wait for device discovery done event
static bool device_discovery_done;
// used to synchronize advertising data handler
static bool advertising_data_received;
// handle to connected Bluetooth device
static bool busy_disconnecting;
static uint16_t conn_handle = NO_CONNECTION;
static uint16_t conn_mtu;

// Bonding status of the peripheral.
static uint16_t bond_auth_err = NO_AUTH;
static uint8_t bond_auth_mode_last = GAPBOND_PAIRING_MODE_NO_PAIRING;

// GATT service handles
static uint16_t gatt_service_handle, gatt_service_end_handle;
// GAP service handles
static uint16_t gap_service_handle, gap_service_end_handle;
// Device information service handles
static uint16_t dev_info_service_handle, dev_info_service_end_handle;
// Pybricks service handles
static uint16_t pybricks_service_handle, pybricks_service_end_handle;
static uint16_t pybricks_command_event_char_handle, pybricks_capabilities_char_handle;
// Pybricks tx notifications enabled
static bool pybricks_notify_en;
// Nordic UART service handles
static uint16_t uart_service_handle, uart_service_end_handle, uart_rx_char_handle, uart_tx_char_handle;
// Nordic UART tx notifications enabled
static bool uart_tx_notify_en;

static const pbdrv_bluetooth_stm32_cc2640_platform_data_t *pdata = &pbdrv_bluetooth_stm32_cc2640_platform_data;

// HACK: restart observing and scanning repeatedly to work around a bug in the
// Bluetooth chip firmware that causes it to stop receiving advertisements
// after a while.
#define SCAN_RESTART_INTERVAL 3000

static pbdrv_bluetooth_peripheral_t peripheral_singleton;

pbdrv_bluetooth_peripheral_t *pbdrv_bluetooth_peripheral_get_by_index(uint8_t index) {
    // This platform supports only a single peripheral instance. Some of its
    // states are global variables listed above. This single instance is used
    // troughout the event handler.
    return &peripheral_singleton;
}

bool pbdrv_bluetooth_peripheral_is_connected(pbdrv_bluetooth_peripheral_t *peri) {
    return peri == &peripheral_singleton && peri->con_handle != NO_CONNECTION;
}

/**
 * Converts a ble error code to the most appropriate pbio error code.
 * @param [in]  status      The ble error code.
 * @return                  The pbio error code.
 */
static pbio_error_t ble_error_to_pbio_error(HCI_StatusCodes_t status) {
    switch (status) {
        case bleSUCCESS:
            return PBIO_SUCCESS;
        case bleInvalidParameter:
            return PBIO_ERROR_INVALID_ARG;
        case bleInvalidEventId:
            return PBIO_ERROR_INVALID_OP;
        case bleNotConnected:
            return PBIO_ERROR_NO_DEV;
        case bleTimeout:
            return PBIO_ERROR_TIMEDOUT;
        default:
            return PBIO_ERROR_FAILED;
    }
}

/**
 * Gets a vendor-specific event payload for @p handle.
 * @param [in]  handle      The connection handle.
 * @param [out] event       The vendor-specific event.
 * @param [out] status      The event status.
 * @param [out] len         The payload length.
 * @return                  The event payload or NULL if there is no pending
 *                          vendor-specific event or the event is for a different
 *                          connection handle.
 */
static uint8_t *get_vendor_event(uint16_t handle, uint16_t *event, HCI_StatusCodes_t *status, uint8_t *len) {
    if (read_buf[NPI_SPI_HEADER_LEN] != HCI_EVENT_PACKET) {
        return NULL;
    }

    if (read_buf[NPI_SPI_HEADER_LEN + 1] != HCI_EVENT_VENDOR_SPECIFIC) {
        return NULL;
    }

    *event = pbio_get_uint16_le(&read_buf[NPI_SPI_HEADER_LEN + 3]);
    *status = read_buf[NPI_SPI_HEADER_LEN + 5];

    if (pbio_get_uint16_le(&read_buf[NPI_SPI_HEADER_LEN + 6]) != handle) {
        return NULL;
    }

    *len = read_buf[NPI_SPI_HEADER_LEN + 8];

    return &read_buf[NPI_SPI_HEADER_LEN + 9];
}

/**
 * Sets the nRESET line on the Bluetooth chip.
 */
static void bluetooth_reset(reset_state_t reset) {
    switch (reset) {
        case RESET_STATE_OUT_HIGH:
            pbdrv_gpio_out_high(&pdata->reset_gpio);
            break;
        case RESET_STATE_OUT_LOW:
            pbdrv_gpio_out_low(&pdata->reset_gpio);
            break;
        case RESET_STATE_INPUT:
            pbdrv_gpio_input(&pdata->reset_gpio);
            break;
    }
}

/**
 * Sets the MRDY signal.
 */
static void spi_set_mrdy(bool mrdy) {
    if (mrdy) {
        pbdrv_gpio_out_low(&pdata->mrdy_gpio);
    } else {
        pbdrv_gpio_out_high(&pdata->mrdy_gpio);
    }
}

const char *pbdrv_bluetooth_get_hub_name(void) {
    return pbdrv_bluetooth_hub_name;
}

const char *pbdrv_bluetooth_get_fw_version(void) {
    return pbdrv_bluetooth_fw_version;
}

/**
 * Sets advertising data and enables advertisements.
 */
pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context) {
    uint8_t data[31];

    PBIO_OS_ASYNC_BEGIN(state);

    DEBUG_PRINT("set_discoverable\n");

    // If we were set to initiate pairing because we bonded with a peripheral
    // previously, unset it to avoid trying to bond with a PC host on connect.
    if (bond_auth_mode_last != GAPBOND_PAIRING_MODE_NO_PAIRING) {
        bond_auth_mode_last = GAPBOND_PAIRING_MODE_NO_PAIRING;
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_BondMgrSetParameter(GAPBOND_PAIRING_MODE, sizeof(bond_auth_mode_last), &bond_auth_mode_last);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    }

    static int8_t tx_power;
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    HCI_LE_readAdvertisingChannelTxPower();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);

    {
        HCI_StatusCodes_t status = read_buf[9];
        if (status == bleSUCCESS) {
            tx_power = read_buf[10];
        }
    }

    // Set advertising data

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    data[0] = 2; // length
    data[1] = GAP_ADTYPE_FLAGS;
    data[2] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
    data[3] = 17; // length
    data[4] = GAP_ADTYPE_128BIT_MORE;
    pbio_uuid128_reverse_copy(&data[5], pbio_pybricks_service_uuid);
    data[21] = 2; // length
    data[22] = GAP_ADTYPE_POWER_LEVEL;
    data[23] = tx_power;
    GAP_updateAdvertisingData(GAP_AD_TYPE_ADVERTISEMENT_DATA, 24, data);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    // Set scan response data

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    data[0] = sizeof(PNP_ID);  // same as 1 + strlen(PNP_ID)
    data[1] = GAP_ADTYPE_SERVICE_DATA;
    memcpy(&data[2], PNP_ID, sizeof(PNP_ID));
    uint8_t hub_name_len = strlen(pbdrv_bluetooth_hub_name);
    data[11] = hub_name_len + 1;
    data[12] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&data[13], pbdrv_bluetooth_hub_name, hub_name_len);
    _Static_assert(13 + sizeof(pbdrv_bluetooth_hub_name) - 1 <= 31, "scan response is 31 octet max\n");

    GAP_updateAdvertisingData(GAP_AD_TYPE_SCAN_RSP_DATA, 13 + hub_name_len, data);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    // make discoverable

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_makeDiscoverable(ADV_IND, GAP_INITIATOR_ADDR_TYPE_PRIVATE_NON_RESOLVE, NULL,
        GAP_CHANNEL_MAP_ALL, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_ADVERTISING_PYBRICKS;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    DEBUG_PRINT("set_non_discoverable or stop broadcasting\n");

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_endDiscoverable();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);

    if (read_buf[8] == bleSUCCESS) {
        // wait for discovery cancel done event
        PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    }
    // Status could also be bleIncorrectMode which means "Not advertising".
    // This is not expected, but should be safe to ignore.

    // This protothread is also shared with stop broadcasting.
    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {

    if (connection == PBDRV_BLUETOOTH_CONNECTION_HCI) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_LE && conn_handle != NO_CONNECTION && !busy_disconnecting) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS && pybricks_notify_en) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_UART && uart_tx_notify_en) {
        return true;
    }

    return false;
}

pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size) {

    static attHandleValueNoti_t notification;

    PBIO_OS_ASYNC_BEGIN(state);

    notification.handle = pybricks_command_event_char_handle,
    notification.pValue = data;
    notification.len = size;

    do {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        ATT_HandleValueNoti(conn_handle, &notification);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    } while ((HCI_StatusCodes_t)read_buf[8] == blePending);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context) {
    pbdrv_bluetooth_peripheral_t *peri = context;

    static pbio_os_timer_t peripheral_scan_restart_timer;

    // Scan and connect timeout, if applicable.
    bool timed_out = peri->config->timeout && pbio_os_timer_is_expired(&peri->timer);

    // Operation can be explicitly cancelled or automatically on inactivity.
    if (!peri->cancel) {
        peri->cancel = pbio_os_timer_is_expired(&peri->watchdog);
    }

    PBIO_OS_ASYNC_BEGIN(state);

    peri->con_handle = NO_CONNECTION;

    // Optionally, disconnect from host (usually Pybricks Code).
    if (conn_handle != NO_CONNECTION &&
        (peri->config->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_DISCONNECT_HOST)) {
        DEBUG_PRINT("Disconnect from Pybricks code (%d).\n", conn_handle);
        // Guard used in pbdrv_bluetooth_is_connected so higher level processes
        // won't try to send anything while we are disconnecting.
        busy_disconnecting = true;
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_TerminateLinkReq(conn_handle, 0x13);
        PBIO_OS_AWAIT_UNTIL(state, conn_handle == NO_CONNECTION);
        busy_disconnecting = false;
    }

restart_scan:

    pbio_os_timer_set(&peripheral_scan_restart_timer, SCAN_RESTART_INTERVAL);

    // start scanning
    DEBUG_PRINT("Start scanning.\n");
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_DeviceDiscoveryRequest(GAP_DEVICE_DISCOVERY_MODE_ALL, 1, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    peri->status = read_buf[8];
    if (peri->status != bleSUCCESS) {
        return ble_error_to_pbio_error(peri->status);
    }

    device_discovery_done = false;

try_again:

    // Wait for advertising data packet.
    for (;;) {
        advertising_data_received = false;
        PBIO_OS_AWAIT_UNTIL(state,
            advertising_data_received || peri->cancel || timed_out ||
            pbio_os_timer_is_expired(&peripheral_scan_restart_timer)
            );

        if (!advertising_data_received) {
            // Things didn't work out, so stop scanning.
            PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
            GAP_DeviceDiscoveryCancel();
            PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
            PBIO_OS_AWAIT_UNTIL(state, device_discovery_done);

            if (peri->cancel) {
                return PBIO_ERROR_CANCELED;
            } else if (timed_out) {
                return PBIO_ERROR_TIMEDOUT;
            } else {
                // Otherwise it's because the sub-scan interval expired, so try again.
                goto restart_scan;
            }
        }

        // Context specific advertisement filter.
        pbdrv_bluetooth_ad_match_result_flags_t adv_flags = peri->config->match_adv(peri->user, read_buf[9], &read_buf[19], NULL, &read_buf[11], peri->bdaddr);

        // If it doesn't match context-specific filter, keep scanning.
        if (!(adv_flags & PBDRV_BLUETOOTH_AD_MATCH_VALUE)) {
            continue;
        }

        // If the value matched but it's the same device as last time, we're
        // here because the scan response failed the last time. It probably
        // won't match now and we should try a different device.
        if (adv_flags & PBDRV_BLUETOOTH_AD_MATCH_ADDRESS) {
            goto try_again;
        }

        // Save the Bluetooth address for later comparison against response.
        peri->bdaddr_type = read_buf[10];
        memcpy(peri->bdaddr, &read_buf[11], 6);
        break;
    }

    // Wait for scan response packet matching previously detected device.
    for (;;) {
        advertising_data_received = false;
        PBIO_OS_AWAIT_UNTIL(state,
            advertising_data_received || peri->cancel || timed_out ||
            pbio_os_timer_is_expired(&peripheral_scan_restart_timer)
            );

        if (!advertising_data_received) {
            // Things didn't work out, so stop scanning.
            PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
            GAP_DeviceDiscoveryCancel();
            PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
            PBIO_OS_AWAIT_UNTIL(state, device_discovery_done);

            if (peri->cancel) {
                return PBIO_ERROR_CANCELED;
            } else if (timed_out) {
                return PBIO_ERROR_TIMEDOUT;
            } else {
                // Otherwise it's because the sub-scan interval expired, so try again.
                goto restart_scan;
            }
        }

        const char *detected_name = (const char *)&read_buf[21];
        const uint8_t *response_address = &read_buf[11];
        pbdrv_bluetooth_ad_match_result_flags_t rsp_flags = peri->config->match_adv_rsp(peri->user, read_buf[9], NULL, detected_name, response_address, peri->bdaddr);

        // If the response data is not right or if the address doesn't match advertisement, keep scanning.
        if (!(rsp_flags & PBDRV_BLUETOOTH_AD_MATCH_VALUE) || !(rsp_flags & PBDRV_BLUETOOTH_AD_MATCH_ADDRESS)) {
            continue;
        }

        // If the device checks passed but the name doesn't match, start over.
        if (rsp_flags & PBDRV_BLUETOOTH_AD_MATCH_NAME_FAILED) {
            goto try_again;
        }

        memcpy(peri->name, detected_name, sizeof(peri->name));
        break;
    }

    // stop scanning
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_DeviceDiscoveryCancel();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    PBIO_OS_AWAIT_UNTIL(state, device_discovery_done);

    // Connect to the peripheral.
    assert(peri->con_handle == NO_CONNECTION);
    bond_auth_err = NO_AUTH;
    DEBUG_PRINT("Found %s. Going to connect.\n", peri->name);

    // Configure to initiate pairing right after connect if bonding required.
    // NB: We must unset "initiate" before we allow a new connection to
    // Pybricks Code or it will try to pair with the PC. We do this just before
    // starting to advertise again, which covers all our use cases.
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    bond_auth_mode_last = (peri->config->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR) ?
        GAPBOND_PAIRING_MODE_INITIATE : GAPBOND_PAIRING_MODE_NO_PAIRING;
    GAP_BondMgrSetParameter(GAPBOND_PAIRING_MODE, sizeof(bond_auth_mode_last), &bond_auth_mode_last);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_EstablishLinkReq(0, 0, peri->bdaddr_type, peri->bdaddr);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    peri->status = read_buf[8];

    // Await connection to be ready unless cancelled sooner.
    PBIO_OS_AWAIT_UNTIL(state, peri->cancel || timed_out || peri->con_handle != NO_CONNECTION);

    if (peri->cancel || timed_out) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_TerminateLinkReq(0xFFFE, 0x13);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    DEBUG_PRINT("Connected.\n");

    if (!(peri->config->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR)) {
        // Pairing not required, so we are done here.
        return PBIO_SUCCESS;
    }

    DEBUG_PRINT("Wait for bonding.\n");
    PBIO_OS_AWAIT_UNTIL(state, peri->cancel || timed_out || bond_auth_err != NO_AUTH);

    if (peri->cancel || timed_out) {
        DEBUG_PRINT("Cancel auth.\n");
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_TerminateAuth(peri->con_handle, 0x13);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    DEBUG_PRINT("Auth complete: 0x%02x\n", bond_auth_err);

    if (bond_auth_err == bleSUCCESS) {
        return PBIO_SUCCESS;
    }

    // bleInvalidEventId:
    // Pairing rejected by peripheral. This can happen if the
    // peripheral has been connected with a different device before
    // we had a chance to delete our bonding info, which can only
    // be done after all connections are closed, including Pybricks Code.
    // bleTimeout:
    // Pairing failed eventually, usually because it got no replies
    // from peripheral at all while connected to Pybricks Code.
    DEBUG_PRINT("Unexpected disconnect.");
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_TerminateLinkReq(peri->con_handle, 0x13);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    PBIO_OS_ASYNC_END(ble_error_to_pbio_error(bond_auth_err));
}

pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context) {
    pbdrv_bluetooth_peripheral_t *peri = context;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    {
        attReadByTypeReq_t req = {
            .startHandle = 0x0001,
            // User upper limit if no end handle specified.
            .endHandle = peri->char_now->handle_max ? peri->char_now->handle_max : 0xFFFF,
        };
        if (peri->char_now->uuid16) {
            pbio_set_uint16_le(req.type.uuid, peri->char_now->uuid16);
            req.type.len = 2;
        } else {
            pbio_uuid128_reverse_copy(req.type.uuid, peri->char_now->uuid128);
            req.type.len = 16;
        }
        GATT_DiscCharsByUUID(peri->con_handle, &req);
    }
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    peri->status = read_buf[8]; // debug

    // GATT_DiscCharsByUUID() sends a "read by type request" so we have to
    // wait until we get an error response to the request or we get a "read
    // by type response" with status of bleProcedureComplete. There can be
    // multiple responses received before the procedure is complete.
    // REVISIT: what happens when remote is disconnected while waiting here?

    PBIO_OS_AWAIT_UNTIL(state, ({
        uint8_t *payload;
        uint8_t len;
        uint16_t event;
        HCI_StatusCodes_t status;
        (payload = get_vendor_event(peri->con_handle, &event, &status, &len)) && ({
            if (event == ATT_EVENT_ERROR_RSP && payload[0] == ATT_READ_BY_TYPE_REQ) {
                return PBIO_ERROR_FAILED;
            }

            event == ATT_EVENT_READ_BY_TYPE_RSP;
        }) && ({
            // hopefully the docs are correct and this is the only possible error
            if (status == bleTimeout) {
                return PBIO_ERROR_TIMEDOUT;
            }

            // this assumes that there is only one matching characteristic with
            // the matching properties. If there is more than one, we will end
            // up with the last. It also assumes that it is the only
            // characteristic in the response.
            if (status == bleSUCCESS) {
                if ((payload[3] & peri->char_now->properties) == peri->char_now->properties) {
                    peri->char_now->handle = pbio_get_uint16_le(&payload[4]);
                    // Don't break out of this even though we found it. We need to
                    // wait until the range scan completes.
                }
            }

            status == bleProcedureComplete;
        });
    }));

    // If notifications are not requested, we're done.
    if (!peri->char_now->request_notification) {
        return PBIO_SUCCESS;
    }

    DEBUG_PRINT("Enable notifications.");
    do {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        static const uint16_t enable = 0x0001;
        // REVISIT: we may want to change this to write with response to ensure
        // that the remote device received the message.
        GATT_WriteNoRsp(peri->con_handle, &(attWriteReq_t) {
            // assuming that client characteristic configuration descriptor
            // is next attribute after the characteristic value attribute
            .handle = peri->char_now->handle + 1,
            .len = sizeof(enable),
            .pValue = (uint8_t *)&enable,
        });
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    } while ((HCI_StatusCodes_t)read_buf[8] == blePending);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = context;

    HCI_StatusCodes_t status;

    PBIO_OS_ASYNC_BEGIN(state);

    DEBUG_PRINT("going to read %04x:\n", peri->char_now->handle);

    do {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        attReadReq_t req = {
            .handle = peri->char_now->handle,
        };
        GATT_ReadCharValue(peri->con_handle, &req);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        status = read_buf[8];

        if (status != bleSUCCESS && status != blePending) {
            return ble_error_to_pbio_error(status);
        }
    } while (status == blePending);

    // This gets a bit tricky. Once the request has been sent, we can't cancel
    // the task, so we have to wait for the response (otherwise the response
    // could be confused with the next request). The device could also become
    // disconnected, in which case we never receive a response.
    PBIO_OS_AWAIT_UNTIL(state, ({
        if (peri->con_handle == NO_CONNECTION) {
            return PBIO_ERROR_NO_DEV;
        }

        uint8_t *payload;
        uint8_t len;
        uint16_t event;
        (payload = get_vendor_event(peri->con_handle, &event, &status, &len)) && ({
            if (event == ATT_EVENT_ERROR_RSP && payload[0] == ATT_READ_REQ
                && pbio_get_uint16_le(&payload[1]) == peri->char_now->handle) {
                return PBIO_ERROR_FAILED;
            }
            // Get the response value.
            if (event == ATT_EVENT_READRSP && len <= sizeof(peri->char_now->value)) {
                peri->char_now->value_len = len;
                memcpy(peri->char_now->value, payload, len);
            }
            event == ATT_EVENT_READRSP;
        });
    }));

    DEBUG_PRINT("Read %04x with status %d\n", peri->char_now->handle, status);

    PBIO_OS_ASYNC_END(ble_error_to_pbio_error(status));
}

pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context) {
    pbdrv_bluetooth_peripheral_t *peri = context;

    HCI_StatusCodes_t status;

    PBIO_OS_ASYNC_BEGIN(state);

    do {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GattWriteCharValue_t req = {
            .connHandle = peri->con_handle,
            .handle = peri->char_write_handle,
            .value = peri->char_write_data,
            .dataSize = peri->char_write_size,
        };
        GATT_WriteCharValue(&req);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

        status = read_buf[8];
        if (status != blePending && status != bleSUCCESS) {
            return ble_error_to_pbio_error(status);
        }

    } while (status == blePending);

    // This gets a bit tricky. Once the request has been sent, we can't cancel
    // the task, so we have to wait for the response (otherwise the response
    // could be confused with the next request). The device could also become
    // disconnected, in which case we never receive a response.

    PBIO_OS_AWAIT_UNTIL(state, ({
        if (peri->con_handle == NO_CONNECTION) {
            return PBIO_ERROR_NO_DEV;
        }

        uint8_t *payload;
        uint8_t len;
        uint16_t event;
        (payload = get_vendor_event(peri->con_handle, &event, &status, &len)) && ({
            if (event == ATT_EVENT_ERROR_RSP && payload[0] == ATT_WRITE_REQ
                && pbio_get_uint16_le(&payload[1]) == peri->char_write_handle) {
                return PBIO_ERROR_FAILED;
            }

            event == ATT_EVENT_WRITE_RSP;
        });
    }));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = context;

    PBIO_OS_ASYNC_BEGIN(state);

    if (peri->con_handle == NO_CONNECTION) {
        return PBIO_ERROR_INVALID_OP;
    }

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_TerminateLinkReq(peri->con_handle, 0x13);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    PBIO_OS_AWAIT_UNTIL(state, peri->con_handle == NO_CONNECTION);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context) {

    PBIO_OS_ASYNC_BEGIN(state);

    // HACK: calling GAP_updateAdvertisingData() repeatedly will cause the
    // Bluetooth chips on Technic and City hubs to eventually lock up. So we
    // call the standard Bluetooth command instead. We still get the vendor-
    // specific command complete event as if we had called the TI command (but
    // not the command status).
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    HCI_LE_setAdvertisingData(pbdrv_bluetooth_broadcast_data_size, pbdrv_bluetooth_broadcast_data);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);

    if (pbdrv_bluetooth_advertising_state == PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING) {
        // Already broadcasting, so just updating the data as above is enough.
        return PBIO_SUCCESS;
    }

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_makeDiscoverable(
        ADV_IND,
        GAP_INITIATOR_ADDR_TYPE_PRIVATE_NON_RESOLVE, NULL,
        GAP_CHANNEL_MAP_ALL, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_WHITELIST);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    if (read_buf[8] != bleSUCCESS) {
        return ble_error_to_pbio_error(read_buf[8]);
    }

    // wait for make discoverable done event
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_DeviceDiscoveryRequest(GAP_DEVICE_DISCOVERY_MODE_NONDISCOVERABLE, 0, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    if (read_buf[8] != bleSUCCESS) {
        return ble_error_to_pbio_error(read_buf[8]);
    }

    device_discovery_done = false;
    pbdrv_bluetooth_is_observing = true;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    if (!pbdrv_bluetooth_is_observing) {
        return PBIO_SUCCESS;
    }

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_DeviceDiscoveryCancel();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    if (read_buf[8] == bleSUCCESS) {
        PBIO_OS_AWAIT_UNTIL(state, device_discovery_done);
    }

    pbdrv_bluetooth_is_observing = false;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// Driver interrupt callbacks

void pbdrv_bluetooth_stm32_cc2640_srdy_irq(bool srdy) {
    spi_srdy = srdy;

    if (!srdy) {
        spi_n_srdy_count++;
    }

    pbio_os_request_poll();
}

void pbdrv_bluetooth_stm32_cc2640_spi_xfer_irq(void) {
    spi_xfer_complete = true;
    pbio_os_request_poll();
}

static void read_by_type_response_uuid16(uint16_t connection_handle,
    uint16_t attr_handle, uint8_t property_flags, uint16_t uuid) {
    attReadByTypeRsp_t rsp;
    uint8_t buf[ATT_MTU_SIZE - 2];

    pbio_set_uint16_le(&buf[0], attr_handle);
    buf[2] = property_flags;
    pbio_set_uint16_le(&buf[3], attr_handle + 1);
    pbio_set_uint16_le(&buf[5], uuid);

    rsp.pDataList = buf;
    rsp.dataLen = 7;
    ATT_ReadByTypeRsp(connection_handle, &rsp);
}

static void read_by_type_response_uuid128(uint16_t connection_handle,
    uint16_t attr_handle, uint8_t property_flags, const uint8_t *uuid) {
    attReadByTypeRsp_t rsp;
    uint8_t buf[ATT_MTU_SIZE - 2];

    pbio_set_uint16_le(&buf[0], attr_handle);
    buf[2] = property_flags;
    pbio_set_uint16_le(&buf[3], attr_handle + 1);
    pbio_uuid128_reverse_copy(&buf[5], uuid);

    rsp.pDataList = buf;
    rsp.dataLen = 21;
    ATT_ReadByTypeRsp(connection_handle, &rsp);
}

// processes an event received from the Bluetooth chip
static void handle_event(uint8_t *packet) {
    uint8_t event = packet[0];
    uint8_t size = packet[1];
    uint8_t *data = &packet[2];
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    (void)size;

    switch (event) {
        case HCI_EVENT_COMMAND_COMPLETE:
            hci_command_complete = true;
            break;

        case HCI_EVENT_VENDOR_SPECIFIC: {
            uint16_t event_code = (data[1] << 8) | data[0];
            HCI_StatusCodes_t status = data[2];
            uint16_t connection_handle = (data[4] << 8) | data[3];
            uint8_t pdu_len = data[5];

            (void)status;
            (void)pdu_len;

            switch (event_code) {
                case ATT_EVENT_EXCHANGE_MTU_REQ: {
                    uint16_t client_mtu = (data[7] << 8) | data[6];

                    // REVISIT: Just saving the main connection MTU for now.
                    // If we allow multiple connections, this will need to be
                    // changed.
                    if (connection_handle == conn_handle) {
                        conn_mtu = MIN(client_mtu, PBDRV_BLUETOOTH_MAX_MTU_SIZE);
                    }

                    attExchangeMTURsp_t rsp;
                    rsp.serverRxMTU = PBDRV_BLUETOOTH_MAX_MTU_SIZE;
                    ATT_ExchangeMTURsp(connection_handle, &rsp);
                }
                break;

                case ATT_EVENT_FIND_BY_TYPE_VALUE_REQ: {
                    uint16_t start_handle = (data[7] << 8) | data[6];
                    uint16_t end_handle = (data[9] << 8) | data[8];
                    uint16_t type = (data[11] << 8) | data[10];
                    uint8_t *value = &data[12];

                    (void)start_handle;
                    (void)end_handle;
                    (void)type;
                    (void)value;

                    DBG("s %04X t %04X", start_handle, type);
                    attErrorRsp_t rsp;

                    rsp.reqOpcode = ATT_FIND_BY_TYPE_VALUE_REQ;
                    rsp.handle = start_handle;
                    rsp.errCode = ATT_ERR_UNSUPPORTED_REQ;
                    ATT_ErrorRsp(connection_handle, &rsp);
                }
                break;

                case ATT_EVENT_READ_BY_TYPE_REQ: {
                    uint16_t start_handle = (data[7] << 8) | data[6];
                    uint16_t end_handle = (data[9] << 8) | data[8];
                    uint16_t type = (data[11] << 8) | data[10];

                    (void)end_handle;

                    DBG("s %04X t %04X", start_handle, type);
                    switch (type) {
                        case GATT_CHARACTER_UUID:
                            if (start_handle <= gap_service_handle + 1) {
                                read_by_type_response_uuid16(connection_handle, gap_service_handle + 1,
                                    GATT_PROP_READ, DEVICE_NAME_UUID);
                            } else if (start_handle <= gap_service_handle + 3) {
                                read_by_type_response_uuid16(connection_handle, gap_service_handle + 3,
                                    GATT_PROP_READ, APPEARANCE_UUID);
                            } else if (start_handle <= gap_service_handle + 5) {
                                read_by_type_response_uuid16(connection_handle, gap_service_handle + 5,
                                    GATT_PROP_READ, PERI_CONN_PARAM_UUID);
                            } else if (start_handle <= dev_info_service_handle + 1) {
                                read_by_type_response_uuid16(connection_handle, dev_info_service_handle + 1,
                                    GATT_PROP_READ, FIRMWARE_REVISION_STRING_UUID);
                            } else if (start_handle <= dev_info_service_handle + 3) {
                                read_by_type_response_uuid16(connection_handle, dev_info_service_handle + 3,
                                    GATT_PROP_READ, SOFTWARE_REVISION_STRING_UUID);
                            } else if (start_handle <= dev_info_service_handle + 5) {
                                read_by_type_response_uuid16(connection_handle, dev_info_service_handle + 5,
                                    GATT_PROP_READ, PNP_ID_UUID);
                            } else if (start_handle <= pybricks_service_handle + 1) {
                                read_by_type_response_uuid128(connection_handle, pybricks_service_handle + 1,
                                    GATT_PROP_WRITE | GATT_PROP_NOTIFY,
                                    pbio_pybricks_command_event_char_uuid);
                            } else if (start_handle <= pybricks_service_handle + 4) {
                                read_by_type_response_uuid128(connection_handle, pybricks_service_handle + 4,
                                    GATT_PROP_READ,
                                    pbio_pybricks_hub_capabilities_char_uuid);
                            } else if (start_handle <= uart_service_handle + 1) {
                                read_by_type_response_uuid128(connection_handle, uart_service_handle + 1,
                                    GATT_PROP_WRITE_NO_RSP, pbio_nus_rx_char_uuid);
                            } else if (start_handle <= uart_service_handle + 3) {
                                read_by_type_response_uuid128(connection_handle, uart_service_handle + 3,
                                    GATT_PROP_NOTIFY, pbio_nus_tx_char_uuid);
                            } else {
                                attErrorRsp_t rsp;

                                rsp.reqOpcode = ATT_READ_BY_TYPE_REQ;
                                rsp.handle = start_handle;
                                rsp.errCode = ATT_ERR_INVALID_VALUE;
                                ATT_ErrorRsp(connection_handle, &rsp);
                            }
                            break;

                        case DEVICE_NAME_UUID: {
                            attReadByTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE - 2];
                            uint8_t hub_name_len = strlen(pbdrv_bluetooth_hub_name);

                            pbio_set_uint16_le(&buf[0], gap_service_handle + 2);
                            memcpy(&buf[2], pbdrv_bluetooth_hub_name, hub_name_len);
                            rsp.pDataList = buf;
                            rsp.dataLen = hub_name_len + 2;
                            ATT_ReadByTypeRsp(connection_handle, &rsp);
                        }
                        break;

                        default: {
                            attErrorRsp_t rsp;

                            rsp.reqOpcode = ATT_READ_BY_TYPE_REQ;
                            rsp.handle = start_handle;
                            rsp.errCode = ATT_ERR_ATTR_NOT_FOUND;
                            ATT_ErrorRsp(connection_handle, &rsp);

                            DBG("unhandled read by type req: %04X", type);
                        }
                        break;
                    }
                }
                break;

                case ATT_EVENT_READ_BY_TYPE_RSP:
                    break;

                case ATT_EVENT_READ_REQ: {
                    uint16_t handle = (data[7] << 8) | data[6];

                    if (handle == gap_service_handle + 2) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        uint8_t hub_name_len = strlen(pbdrv_bluetooth_hub_name);
                        memcpy(&buf[0], pbdrv_bluetooth_hub_name, hub_name_len);
                        rsp.len = hub_name_len;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == gap_service_handle + 4) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        pbio_set_uint16_le(&buf[0], GAP_APPEARE_UNKNOWN);
                        rsp.len = 2;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == gap_service_handle + 6) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        // FIXME: what should these values be?
                        pbio_set_uint16_le(&buf[0], 0xFFFF); // intervalMin
                        pbio_set_uint16_le(&buf[2], 0xFFFF); // intervalMax
                        pbio_set_uint16_le(&buf[4], 0xFFFF); // latency
                        pbio_set_uint16_le(&buf[6], 0xFFFF); // timeout
                        rsp.len = 8;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == dev_info_service_handle + 2) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        memcpy(&buf[0], PBIO_VERSION_STR, sizeof(PBIO_VERSION_STR) - 1);
                        rsp.len = sizeof(PBIO_VERSION_STR) - 1;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == dev_info_service_handle + 4) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        memcpy(&buf[0], PBIO_PROTOCOL_VERSION_STR, sizeof(PBIO_PROTOCOL_VERSION_STR) - 1);
                        rsp.len = sizeof(PBIO_PROTOCOL_VERSION_STR) - 1;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == dev_info_service_handle + 6) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        buf[0] = 0x01; // Vendor ID Source Field - Bluetooth SIG-assigned ID
                        pbio_set_uint16_le(&buf[1], LWP3_LEGO_COMPANY_ID); // Vendor ID Field
                        pbio_set_uint16_le(&buf[3], PBDRV_CONFIG_BLUETOOTH_STM32_CC2640_HUB_ID[0]); // Product ID Field
                        pbio_set_uint16_le(&buf[5], 0); // Product Version Field
                        rsp.len = 7;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == pybricks_command_event_char_handle + 1) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        buf[0] = pybricks_notify_en;
                        buf[1] = 0;
                        rsp.len = 2;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == pybricks_capabilities_char_handle + 1) {
                        attReadRsp_t rsp;
                        uint8_t buf[PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE];

                        // REVISIT: this assumes connection_handle == conn_handle
                        pbio_pybricks_hub_capabilities(buf, conn_mtu - 3, PBSYS_CONFIG_APP_FEATURE_FLAGS, pbsys_storage_get_maximum_program_size(), 0);
                        rsp.len = sizeof(buf);
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else if (handle == uart_tx_char_handle + 1) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        buf[0] = uart_tx_notify_en;
                        buf[1] = 0;
                        rsp.len = 2;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    } else {
                        DBG("unhandled read req: %04X", handle);
                    }
                }
                break;

                case ATT_EVENT_READ_BY_GRP_TYPE_REQ: {
                    uint16_t start_handle = (data[7] << 8) | data[6];
                    uint16_t end_handle = (data[9] << 8) | data[8];
                    uint16_t group_type = (data[11] << 8) | data[10];

                    (void)end_handle;

                    DBG("s %04X g %04X", start_handle, group_type);
                    switch (group_type) {
                        case GATT_PRIMARY_SERVICE_UUID:
                            if (start_handle <= gatt_service_handle) {
                                attReadByGrpTypeRsp_t rsp;
                                uint8_t buf[ATT_MTU_SIZE - 2];

                                pbio_set_uint16_le(&buf[0], gatt_service_handle);
                                pbio_set_uint16_le(&buf[2], gatt_service_end_handle);
                                pbio_set_uint16_le(&buf[4], GATT_SERVICE_UUID);

                                rsp.pDataList = buf;
                                rsp.dataLen = 6;
                                ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                            } else if (start_handle <= gap_service_handle) {
                                attReadByGrpTypeRsp_t rsp;
                                uint8_t buf[ATT_MTU_SIZE - 2];

                                pbio_set_uint16_le(&buf[0], gap_service_handle);
                                pbio_set_uint16_le(&buf[2], gap_service_end_handle);
                                pbio_set_uint16_le(&buf[4], GAP_SERVICE_UUID);

                                rsp.pDataList = buf;
                                rsp.dataLen = 6;
                                ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                            } else if (start_handle <= dev_info_service_handle) {
                                attReadByGrpTypeRsp_t rsp;
                                uint8_t buf[ATT_MTU_SIZE - 2];

                                pbio_set_uint16_le(&buf[0], dev_info_service_handle);
                                pbio_set_uint16_le(&buf[2], dev_info_service_end_handle);
                                pbio_set_uint16_le(&buf[4], DI_SERVICE_UUID);

                                rsp.pDataList = buf;
                                rsp.dataLen = 6;
                                ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                            } else if (start_handle <= pybricks_service_handle) {
                                attReadByGrpTypeRsp_t rsp;
                                uint8_t buf[ATT_MTU_SIZE - 2];

                                pbio_set_uint16_le(&buf[0], pybricks_service_handle);
                                pbio_set_uint16_le(&buf[2], pybricks_service_end_handle);
                                pbio_uuid128_reverse_copy(&buf[4], pbio_pybricks_service_uuid);

                                rsp.pDataList = buf;
                                rsp.dataLen = 20;
                                ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                            } else if (start_handle <= uart_service_handle) {
                                attReadByGrpTypeRsp_t rsp;
                                uint8_t buf[ATT_MTU_SIZE - 2];

                                pbio_set_uint16_le(&buf[0], uart_service_handle);
                                pbio_set_uint16_le(&buf[2], uart_service_end_handle);
                                pbio_uuid128_reverse_copy(&buf[4], pbio_nus_service_uuid);

                                rsp.pDataList = buf;
                                rsp.dataLen = 20;
                                ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                            } else {
                                attErrorRsp_t rsp;

                                rsp.reqOpcode = ATT_READ_BY_GRP_TYPE_REQ;
                                rsp.handle = start_handle;
                                rsp.errCode = ATT_ERR_ATTR_NOT_FOUND;
                                ATT_ErrorRsp(connection_handle, &rsp);
                            }
                            break;
                        default: {
                            attErrorRsp_t rsp;

                            rsp.reqOpcode = ATT_READ_BY_GRP_TYPE_REQ;
                            rsp.handle = start_handle;
                            rsp.errCode = ATT_ERR_INVALID_VALUE;
                            ATT_ErrorRsp(connection_handle, &rsp);

                            DBG("unhandled read by grp type req: %05X", group_type);
                        }
                        break;
                    }
                }
                break;

                case ATT_EVENT_WRITE_REQ: {
                    uint8_t command = data[7]; // command = write without response
                    uint16_t char_handle = (data[9] << 8) | data[8];
                    pbio_pybricks_error_t err = PBIO_PYBRICKS_ERROR_INVALID_HANDLE;

                    DBG("w: %04X %04X %d", char_handle, uart_tx_char_handle, pdu_len - 4);
                    if (char_handle == pybricks_command_event_char_handle) {
                        if (pbdrv_bluetooth_receive_handler) {
                            err = pbdrv_bluetooth_receive_handler(&data[10], pdu_len - 4);
                        }
                    } else if (char_handle == pybricks_command_event_char_handle + 1) {
                        pybricks_notify_en = data[10];
                        err = PBIO_PYBRICKS_ERROR_OK;
                        DBG("noti: %d", pybricks_notify_en);
                    } else if (char_handle == uart_rx_char_handle) {
                        // Not implemented
                    } else if (char_handle == uart_tx_char_handle + 1) {
                        uart_tx_notify_en = data[10];
                        err = PBIO_PYBRICKS_ERROR_OK;
                        DBG("noti: %d", uart_tx_notify_en);
                    } else {
                        DBG("unhandled write req: %04X", char_handle);
                    }

                    if (!command) {
                        if (err) {
                            attErrorRsp_t rsp;

                            rsp.reqOpcode = ATT_WRITE_REQ;
                            rsp.handle = char_handle;
                            rsp.errCode = err;
                            ATT_ErrorRsp(connection_handle, &rsp);
                        } else {
                            ATT_WriteRsp(connection_handle);
                        }
                    }
                }
                break;

                case ATT_EVENT_PREPARE_WRITE_REQ: {
                    uint16_t char_handle = (data[7] << 8) | data[6];

                    // long writes are not currently supported for any characteristic
                    attErrorRsp_t rsp;
                    rsp.reqOpcode = ATT_PREPARE_WRITE_REQ;
                    rsp.handle = char_handle;
                    rsp.errCode = ATT_ERR_ATTR_NOT_LONG;
                    ATT_ErrorRsp(connection_handle, &rsp);
                }
                break;

                case ATT_EVENT_EXECUTE_WRITE_REQ: {
                    // uint8_t flags = data[6];

                    // Send this to keep host happy. Should only be receiving
                    // cancel since we don't support long writes.
                    ATT_ExecuteWriteRsp(connection_handle);
                }
                break;

                case ATT_EVENT_HANDLE_VALUE_NOTI: {
                    // TODO: match callback to handle
                    // uint8_t attr_handle = (data[7] << 8) | data[6];
                    if (peri->config->notification_handler) {
                        peri->config->notification_handler(peri->user, &data[8], pdu_len - 2);
                    }
                }
                break;

                case GAP_DEVICE_DISCOVERY_DONE:
                    device_discovery_done = true;
                    break;

                case GAP_LINK_ESTABLISHED:
                    if (data[12] == GAP_PROFILE_PERIPHERAL) {
                        // we currently only allow connection from one central
                        conn_handle = (data[11] << 8) | data[10];
                        DBG("link: %04x", conn_handle);
                        // assume minimum MTU until we get an exchange MTU request
                        conn_mtu = ATT_MTU_SIZE;

                        // Establishing the link implicitly stops advertising.
                        pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;

                        // On 2019 and newer MacBooks, the default interval was
                        // measured to be 15 ms. This caused advertisement to
                        // not be received by the local Bluetooth chip when
                        // scanning for devices. Calling the command below
                        // effectively sends the L2CAP Connection Parameter
                        // Update Request suggested by Apple[1] to reduce the
                        // interval to presumably make more room for receiving
                        // advertising data. There are a number of requirements
                        // in the Apple spec, such as the interval must be a
                        // multiple of 15 ms.
                        // [1]: https://developer.apple.com/accessories/Accessory-Design-Guidelines.pdf
                        gapUpdateLinkParamReq_t req = {
                            .connectionHandle = conn_handle,
                            .intervalMin = 24, // 24 * 1.25 ms = 30 ms
                            .intervalMax = 48, // 48 * 1.25 ms = 60 ms
                            .connLatency = 0,
                            .connTimeout = 500, // 500 * 10 ms = 5 s
                        };
                        GAP_UpdateLinkParamReq(&req);
                    } else if (data[12] == GAP_PROFILE_CENTRAL) {
                        // we currently only allow connection to one LEGO Powered Up remote peripheral
                        peri->con_handle = (data[11] << 8) | data[10];
                    }
                    break;

                case GAP_LINK_TERMINATED: {
                    DBG("bye: %04x", connection_handle);
                    if (conn_handle == connection_handle) {
                        conn_handle = NO_CONNECTION;
                        pybricks_notify_en = false;
                        uart_tx_notify_en = false;
                    } else if (peri->con_handle == connection_handle) {
                        peri->con_handle = NO_CONNECTION;
                    }
                }
                break;

                case GAP_LINK_PARAM_UPDATE:
                    // we get this event, but don't need to do anything about it
                    break;

                case GAP_DEVICE_INFORMATION:
                    if (pbdrv_bluetooth_observe_callback) {
                        uint8_t event_type = data[3];
                        // uint8_t addr_type = data[4];
                        // uint8_t *addr = &data[5];
                        int8_t rssi = data[11];
                        uint8_t data_len = data[12];
                        uint8_t *data_field = &data[13];

                        pbdrv_bluetooth_observe_callback(event_type, data_field, data_len, rssi);
                    }

                    advertising_data_received = true;
                    break;

                case GAP_DEVICE_INIT_DONE:
                case HCI_EXT_SET_TX_POWER_EVENT:
                case HCI_EXT_SET_LOCAL_SUPPORTED_FEATURES_EVENT:
                case HCI_EXT_SET_BDADDR_EVENT:
                case GAP_ADVERT_DATA_UPDATE_DONE:
                case GAP_MAKE_DISCOVERABLE_DONE:
                case GAP_END_DISCOVERABLE_DONE:
                    hci_command_complete = true;
                    break;

                case GAP_BOND_ERR_OFFSET + GAP_SIGNATURE_UPDATED_EVENT:
                case GAP_BOND_ERR_OFFSET + GAP_PASSKEY_NEEDED_EVENT:
                case GAP_BOND_ERR_OFFSET + GAP_SLAVE_REQUESTED_SECURITY_EVENT:
                case GAP_BOND_ERR_OFFSET + GAP_PAIRING_REQ_EVENT:
                case GAP_BOND_ERR_OFFSET + GAP_AUTHENTICATION_FAILURE_EVT:
                case GAP_BOND_ERR_OFFSET + GAP_BOND_COMPLETE_EVENT:
                    DEBUG_PRINT("Bond info: 0x%04X %d\n", event_code, status);
                    bond_auth_err = status;
                    break;

                case GAP_BOND_ERR_OFFSET + GAP_AUTHENTICATION_COMPLETE_EVENT:
                    DEBUG_PRINT("Auth complete: 0x%04X %d\n", event_code, status);
                    bond_auth_err = status;
                    break;

                case HCI_COMMAND_STATUS: {
                    uint16_t opcode = (data[4] << 8) | data[3];
                    // This filters out responses that were sent from commands
                    // initiated from here in the event handler.
                    if (opcode == hci_command_opcode) {
                        hci_command_status = true;
                    }
                    if (status != bleSUCCESS) {
                        DBG("status: %02X %04X", status, connection_handle);
                    }
                }
                break;

                default:
                    DBG("unhandled: %04X", event_code);
                    DEBUG_PRINT("unhandled: %04X\n", event_code);
                    break;
            }
        }
        break;
    }

    pbio_os_request_poll();
}

// gets the number of bytes remaining to be read, if any
static bool get_npi_rx_size(uint8_t *rx_size) {
    if (read_buf[1] != NPI_SPI_SOF) {
        return false;
    }

    // total read size is payload length plus 1 FCS byte
    *rx_size = read_buf[2] + 1;

    return true;
}

typedef struct {
    const uint8_t *uuid128; // Null means uuid16 is used.
    uint16_t uuid16;
    uint8_t permissions;
} gatt_add_attribute_t;

static const gatt_add_attribute_t gatt_init_attributes[] = {
    {
        .uuid16 = GATT_CHARACTER_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid16 = DEVICE_NAME_UUID,
        .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    },
    {
        .uuid16 = GATT_CHARACTER_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid16 = APPEARANCE_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid16 = GATT_CHARACTER_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid16 = PERI_CONN_PARAM_UUID,
        .permissions = GATT_PERMIT_READ,
    },
};

static pbio_error_t gatt_add_attributes(pbio_os_state_t *state, const gatt_add_attribute_t *attrs, size_t num_attrs) {
    static size_t i;

    PBIO_OS_ASYNC_BEGIN(state);

    for (i = 0; i < num_attrs; i++) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        const gatt_add_attribute_t *attr = &attrs[i];
        if (attr->uuid128) {
            GATT_AddAttribute2(attr->uuid128, attr->permissions);
        } else {
            GATT_AddAttribute(attr->uuid16, attr->permissions);
        }
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_error_t gatt_init(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 1, GATT_MIN_ENCRYPT_KEY_SIZE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    gatt_service_handle = (read_buf[13] << 8) | read_buf[12];
    gatt_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    DBG("gatt: %04X", gatt_service_handle);

    /**************************************************************************/

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 7, GATT_MIN_ENCRYPT_KEY_SIZE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    // ignoring response data

    PBIO_OS_AWAIT(state, &sub, gatt_add_attributes(&sub, gatt_init_attributes, PBIO_ARRAY_SIZE(gatt_init_attributes)));

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    gap_service_handle = (read_buf[13] << 8) | read_buf[12];
    gap_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    DBG("gap: %04X", gap_service_handle);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static const struct {
    uint8_t param_id;
    uint16_t value;
} gap_params[] = {
    // Set scan timeout on btchip to infinite. Separate timeout
    // is implemented to stop program and thus scan
    {TGAP_GEN_DISC_SCAN, 0},
    // Set scan timeout on btchip to infinite. Separate timeout
    // is implemented to stop program and thus scan
    {TGAP_LIM_DISC_SCAN, 0},
    {TGAP_GEN_DISC_ADV_INT_MIN, 40},
    {TGAP_GEN_DISC_ADV_INT_MAX, 40},
    {TGAP_CONN_ADV_INT_MIN, 40},
    {TGAP_CONN_ADV_INT_MAX, 40},
    // scan interval general discovery: 48 * 0.625ms = 30ms
    {TGAP_GEN_DISC_SCAN_INT, 48},
    // scan window general discovery: 48 * 0.625ms = 30ms
    {TGAP_GEN_DISC_SCAN_WIND, 48},
    {TGAP_CONN_EST_INT_MIN, 40},
    {TGAP_CONN_EST_INT_MAX, 40},
    // scan interval connection established: 48 * 0.625ms = 30ms
    {TGAP_CONN_EST_SCAN_INT, 48},
    // scan window connection established: 48 * 0.625ms = 30ms
    {TGAP_CONN_EST_SCAN_WIND, 48},
    {TGAP_CONN_EST_SUPERV_TIMEOUT, 60},
    // When acting as a central, make the hub reject connection parameter
    // changes requested by peripherals. This ensures that things like
    // broadcasting keep working. May need to revisit this if we ever have
    // peripherals that only work with their own connection parameters.
    {TGAP_REJECT_CONN_PARAMS, 1},
    {TGAP_CONN_EST_LATENCY, 0},
    {TGAP_CONN_EST_MIN_CE_LEN, 4},
    {TGAP_CONN_EST_MAX_CE_LEN, 4},
    {TGAP_FILTER_ADV_REPORTS, 0},
};

static const struct {
    uint16_t param_id;
    uint8_t value;
} bond_params[] = {
    {GAPBOND_PAIRING_MODE, GAPBOND_PAIRING_MODE_NO_PAIRING},
    {GAPBOND_MITM_PROTECTION, 0}, // disabled
    {GAPBOND_IO_CAPABILITIES, GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT},
    {GAPBOND_BONDING_ENABLED, 1}, // enabled, as in allowed. It won't happen by default.
    {GAPBOND_SECURE_CONNECTION, GAPBOND_SECURE_CONNECTION_NONE},
};

static pbio_error_t gap_init(pbio_os_state_t *state, void *context) {

    static size_t idx;

    PBIO_OS_ASYNC_BEGIN(state);

    // Configure bond manager.
    for (idx = 0; idx < PBIO_ARRAY_SIZE(bond_params); idx++) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_BondMgrSetParameter(bond_params[idx].param_id, 1, (uint8_t *)&bond_params[idx].value);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        // ignoring response data
    }

    // Read the number of bonds stored in flash.
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_BondMgrGetParameter(GAPBOND_BOND_COUNT);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

    // Connecting to previously bonded devices does not always work reliably
    // due to particular behaviors of some peripherals, combined with the
    // random address used by Pybricks. Bonding takes only a few seconds, so we
    // just always clear the bond information if there is any, and start over.
    if (read_buf[12] > 0) {
        DEBUG_PRINT("Old bond count: %d\n", read_buf[12]);

        // Erase all bonds stored on Bluetooth chip. We can also erase local
        // info, but this does not appear to be necessary.
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_BondMgrSetParameter(GAPBOND_ERASE_ALLBONDS, 0, NULL);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);

        // Read the number of bonds stored in flash, should now be zero.
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_BondMgrGetParameter(GAPBOND_BOND_COUNT);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        DEBUG_PRINT("New bond count: %d\n", read_buf[12]);
    }

    // Configure GAP parameters.
    for (idx = 0; idx < PBIO_ARRAY_SIZE(gap_params); idx++) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_SetParamValue(gap_params[idx].param_id, gap_params[idx].value);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        // ignoring response data
    }

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GAP_deviceInit(GAP_PROFILE_PERIPHERAL | GAP_PROFILE_CENTRAL, 8, NULL, NULL, 0);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    // This sets the device address to a new random value each time we reset
    // the Bluetooth chip.
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    {
        // NB: there doesn't seem to be a way to get random bytes from the
        // Bluetooth chip itself.
        uint32_t random[2];
        pbdrv_random_get(random);
        // REVISIT: should check return value and use second call to get more
        // randomness
        random[1] = ~random[0];

        GAP_ConfigDeviceAddr(GAP_INITIATOR_ADDR_TYPE_PRIVATE_NON_RESOLVE, (uint8_t *)random);
    }
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    // ignoring response data

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// Initializes the Bluetooth chip
// this function is largely inspired by the LEGO bootloader
static pbio_error_t hci_init(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    // // set the Bluetooth address

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    HCI_EXT_setBdaddr(pdata->bd_addr);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    // read Bluetooth address

    // TODO: skip getting bdaddr - it is returned by both the previous and next
    // commands anyway

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    HCI_readBdaddr();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    // set Tx power level

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    HCI_EXT_setTxPower(HCI_EXT_CC26XX_TX_POWER_0_DBM);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    HCI_readLocalVersionInfo();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    // firmware version is TI BLE-Stack SDK version
    snprintf(pbdrv_bluetooth_fw_version, sizeof(pbdrv_bluetooth_fw_version),
        "v%u.%02u.%02u", read_buf[12], read_buf[11] >> 4, read_buf[11] & 0xf);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    HCI_EXT_setLocalSupportedFeatures(HCI_EXT_LOCAL_FEATURE_ENCRYTION);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // ignoring response data

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    Util_readLegoHwVersion();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    // ignoring response data

    // REVISIT: LEGO Technic hub firmware uses this as hub HW version

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    Util_readLegoFwVersion();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    // ignoring response data

    // REVISIT: LEGO Technic hub firmware appends this to radio FW version

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static const uint16_t gat_uuid_permit_read[] = {
    GATT_CHARACTER_UUID,
    FIRMWARE_REVISION_STRING_UUID,
    GATT_CHARACTER_UUID,
    SOFTWARE_REVISION_STRING_UUID,
    GATT_CHARACTER_UUID,
    // Should be last. This is the only one where we read the response.
    PNP_ID_UUID,
};

static pbio_error_t init_device_information_service(pbio_os_state_t *state, void *context) {

    static size_t idx;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 7, GATT_MIN_ENCRYPT_KEY_SIZE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    // ignoring response data

    for (idx = 0; idx < PBIO_ARRAY_SIZE(gat_uuid_permit_read); idx++) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GATT_AddAttribute(gat_uuid_permit_read[idx], GATT_PERMIT_READ);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        // ignoring response data
    }

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    dev_info_service_handle = (read_buf[13] << 8) | read_buf[12];
    dev_info_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    DBG("device information: %04X", dev_info_service_handle);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static const gatt_add_attribute_t pybricks_gatt_attributes[] = {
    {
        .uuid16 = GATT_CHARACTER_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid128 = pbio_pybricks_command_event_char_uuid,
        .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    },
    {
        .uuid16 = GATT_CLIENT_CHAR_CFG_UUID,
        .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    },
    {
        .uuid16 = GATT_CHARACTER_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid128 = pbio_pybricks_hub_capabilities_char_uuid,
        .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    },
};

static pbio_error_t init_pybricks_service(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 6, GATT_MIN_ENCRYPT_KEY_SIZE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    // ignoring response data

    PBIO_OS_AWAIT(state, &sub, gatt_add_attributes(&sub, pybricks_gatt_attributes, PBIO_ARRAY_SIZE(pybricks_gatt_attributes)));

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    pybricks_service_handle = (read_buf[13] << 8) | read_buf[12];
    pybricks_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    pybricks_command_event_char_handle = pybricks_service_handle + 2;
    pybricks_capabilities_char_handle = pybricks_service_handle + 4;
    DBG("pybricks: %04X", pybricks_service_handle);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static const gatt_add_attribute_t uart_gatt_attributes[] = {
    {
        .uuid16 = GATT_CHARACTER_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid128 = pbio_nus_rx_char_uuid,
        .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    },
    {
        .uuid16 = GATT_CHARACTER_UUID,
        .permissions = GATT_PERMIT_READ,
    },
    {
        .uuid128 = pbio_nus_tx_char_uuid,
        .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    },
    {
        .uuid16 = GATT_CLIENT_CHAR_CFG_UUID,
        .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE,
    },
};

static pbio_error_t init_uart_service(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // add the Nordic UART service (inspired by Add_Sample_Service() from
    // sample_service.c in BlueNRG vendor sample code and Adafruit config file
    // https://github.com/adafruit/Adafruit_nRF8001/blob/master/utility/uart/UART_over_BLE.xml)

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 6, GATT_MIN_ENCRYPT_KEY_SIZE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    // ignoring response data

    PBIO_OS_AWAIT(state, &sub, gatt_add_attributes(&sub, uart_gatt_attributes, PBIO_ARRAY_SIZE(uart_gatt_attributes)));

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    uart_service_handle = (read_buf[13] << 8) | read_buf[12];
    uart_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    uart_rx_char_handle = uart_service_handle + 2;
    uart_tx_char_handle = uart_service_handle + 4;
    DBG("uart: %04X", uart_service_handle);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_controller_reset_hard(void) {
    pybricks_notify_en = uart_tx_notify_en = false;
    conn_handle = peripheral_singleton.con_handle = NO_CONNECTION;

    spi_set_mrdy(false);
    bluetooth_reset(RESET_STATE_OUT_LOW);
}

pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    PBIO_OS_ASYNC_BEGIN(state);

    // Disconnect gracefully if connected to host.
    if (conn_handle != NO_CONNECTION) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        GAP_TerminateLinkReq(conn_handle, 0x13);
        PBIO_OS_AWAIT_UNTIL(state, conn_handle == NO_CONNECTION);
    }

    pbdrv_bluetooth_controller_reset_hard();

    PBIO_OS_AWAIT_MS(state, timer, 150);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    static pbio_os_state_t sub;
    static size_t idx;

    PBIO_OS_ASYNC_BEGIN(state);

    // Take Bluetooth chip out of reset. Unclear why input state is needed.
    bluetooth_reset(RESET_STATE_OUT_HIGH);
    PBIO_OS_AWAIT_MS(state, timer, 150);
    bluetooth_reset(RESET_STATE_INPUT);
    PBIO_OS_AWAIT_MS(state, timer, 100);

    // Bluetooth chip should not have any messages for us yet, so spi_srdy
    // should be false, if not wait a while and try resetting the chip.
    if (spi_srdy) {
        pbio_os_timer_set(timer, 500);
        PBIO_OS_AWAIT_UNTIL(state, !spi_srdy || pbio_os_timer_is_expired(timer));
        if (spi_srdy) {
            // This will make main Bluetooth thread reset and retry.
            return PBIO_ERROR_IO;
        }
    }

    // Init hci, gatt, gap, and services.
    static const pbio_os_process_func_t init_funcs[] = {
        hci_init,
        gatt_init,
        gap_init,
        init_device_information_service,
        init_pybricks_service,
        init_uart_service,
    };
    for (idx = 0; idx < PBIO_ARRAY_SIZE(init_funcs); idx++) {
        PBIO_OS_AWAIT(state, &sub, init_funcs[idx](&sub, NULL));
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_os_process_t pbdrv_bluetooth_spi_process;

static pbio_error_t pbdrv_bluetooth_spi_process_thread(pbio_os_state_t *state, void *context) {
    static uint8_t read_xfer_size, xfer_size;

    static pbio_os_state_t main_thread_state;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_ASYNC_RESET(&main_thread_state);

    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, {
            // Iterate the main Bluetooth thread once when some data is
            // available or new data can be sent.
            pbio_error_t err = pbdrv_bluetooth_process_thread(&main_thread_state, NULL);

            // Exit this spi process once the high level Bluetooth process completes.
            if (err != PBIO_ERROR_AGAIN) {
                return err;
            }

            // - spi_srdy is set by the interrupt handler when there is new
            //   data to be processed, which also polls this process in order
            //   to push PROCESS_WAIT_UNTIL to evaluate this condition again.
            // - write_xfer_size is set by HCI_sendHCICommand when main thread
            //   driven above above wants to send something. This doesn't poll
            //   the process because we are already here.
            spi_srdy || write_xfer_size;
        });

        spi_set_mrdy(true);

        // Need to make a copy of the write transfer size for the case where we
        // are reading only. In this case we still have to set write_xfer_size
        // to a non-zero value to indicate that write_buf is being used. But we
        // still need the real size later to figure out the actual size of the
        // SPI transfer.
        static uint8_t real_write_xfer_size;
        real_write_xfer_size = write_xfer_size;

        // we can either read, write, or read and write at the same time

        if (real_write_xfer_size) {
            // if we are writing only, we have to wait until SRDY is asserted
            PBIO_OS_AWAIT_UNTIL(state, spi_srdy);
        } else {
            // if we are reading only, the write buffer has to be all 0s
            memset(write_buf, 0, PBIO_ARRAY_SIZE(write_buf));
            // indicates that write_buf is in use
            write_xfer_size = PBIO_ARRAY_SIZE(write_buf);
        }

        // send the write header
        spi_xfer_complete = false;
        pdata->spi_start_xfer(write_buf, read_buf, NPI_SPI_HEADER_LEN);
        PBIO_OS_AWAIT_UNTIL(state, spi_xfer_complete);

        // Total transfer size is biggest of read and write sizes.
        read_xfer_size = 0;
        if (get_npi_rx_size(&read_xfer_size) && read_xfer_size + NPI_SPI_HEADER_LEN > real_write_xfer_size) {
            xfer_size = read_xfer_size;
        } else {
            // remaining size is write size - 3 bytes already sent
            xfer_size = real_write_xfer_size - NPI_SPI_HEADER_LEN;

        }

        // read the remaining message
        spi_xfer_complete = false;
        pdata->spi_start_xfer(&write_buf[NPI_SPI_HEADER_LEN],
            &read_buf[NPI_SPI_HEADER_LEN], xfer_size);
        PBIO_OS_AWAIT_UNTIL(state, spi_xfer_complete);

        // After the transfer is complete, we release the MRDY signal to the
        // Bluetooth chip and it acknowledges by releasing the SRDY signal.
        static uint8_t prev_n_srdy_count;
        prev_n_srdy_count = spi_n_srdy_count;
        spi_set_mrdy(false);
        // Multiple interrupts can happen between line above and line below, so
        // we can't use PBIO_OS_AWAIT_UNTIL(state, !spi_srdy) as the SRDY
        // signal may go off and back on again before we can read it once. So
        // we have a separate falling edge trigger to ensure we catch the
        // event.
        PBIO_OS_AWAIT_UNTIL(state, spi_n_srdy_count != prev_n_srdy_count);

        // set to 0 to indicate that xfer is complete
        write_xfer_size = 0;

        if (read_xfer_size) {
            // handle the received data
            if (read_buf[NPI_SPI_HEADER_LEN] == HCI_EVENT_PACKET) {
                handle_event(&read_buf[NPI_SPI_HEADER_LEN + 1]);
            }
        }
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

// Implements function for bt5stack library. Doesn't send right away, but gets
// data ready and sets write_xfer_size so that bluetooth driver process can
// send it. It is only ever called from within the bluetooth process, so this
// doesn't need to poll the process to push it along.
HCI_StatusCodes_t HCI_sendHCICommand(uint16_t opcode, uint8_t *payload_data, uint8_t data_length) {
    assert(write_xfer_size == 0);

    write_buf[0] = NPI_SPI_SOF;
    write_buf[1] = data_length + 4;
    write_buf[2] = HCI_CMD_PACKET;
    pbio_set_uint16_le(&write_buf[3], opcode);
    write_buf[5] = data_length;
    if (payload_data) {
        memcpy(&write_buf[6], payload_data, data_length);
    }

    // calculate and append FCS byte
    uint8_t checksum = 0;
    for (int i = 1; i < data_length + 6; i++) {
        checksum ^= write_buf[i];
    }
    write_buf[data_length + 6] = checksum;

    write_xfer_size = data_length + 7;

    // NB: some commands only receive CommandStatus, others only receive specific
    // reply, others receive both
    hci_command_opcode = opcode;
    hci_command_status = false;
    hci_command_complete = false;

    return bleSUCCESS;
}

void pbdrv_bluetooth_init_hci(void) {
    pbdrv_gpio_set_pull(&pdata->reset_gpio, PBDRV_GPIO_PULL_NONE);
    bluetooth_reset(RESET_STATE_OUT_LOW);

    pbdrv_gpio_set_pull(&pdata->mrdy_gpio, PBDRV_GPIO_PULL_NONE);
    spi_set_mrdy(false);

    pdata->spi_init();

    pbio_os_process_start(&pbdrv_bluetooth_spi_process, pbdrv_bluetooth_spi_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLUETOOTH_STM32_CC2640
