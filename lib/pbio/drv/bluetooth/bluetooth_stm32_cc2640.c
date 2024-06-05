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
#include <pbdrv/gpio.h>
#include <pbdrv/random.h>
#include <pbio/error.h>
#include <pbio/protocol.h>
#include <pbio/task.h>
#include <pbio/util.h>
#include <pbio/version.h>
#include <pbsys/app.h>
#include <pbsys/storage.h>

#include <contiki.h>
#include <contiki-lib.h>
#include <lego_lwp3.h>

#include <att.h>
#include <gap.h>
#include <gatt.h>
#include <gattservapp.h>
#include <hci_ext.h>
#include <hci_tl.h>
#include <hci.h>
#include <util.h>

#include "./bluetooth_stm32_cc2640.h"

#define DEBUG_LL (0x01)
#define DEBUG_PT (0x02)

// Choose either/or DEBUG_LL | DEBUG_PT
#define DEBUG (0)

#if DEBUG
#include <pbdrv/../../drv/ioport/ioport_debug_uart.h>
#endif
#if (DEBUG & DEBUG_LL)
#define DBG pbdrv_ioport_debug_uart_printf
#else
#define DBG(...)
#endif
#if (DEBUG & DEBUG_PT)
#define DEBUG_PRINT pbdrv_ioport_debug_uart_printf
#define DEBUG_PRINT_PT PBDRV_IOPORT_DEBUG_UART_PT_PRINTF
#else
#define DEBUG_PRINT_PT(...)
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

static bool is_broadcasting;
static bool is_observing;
static pbdrv_bluetooth_start_observing_callback_t observe_callback;

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

// The peripheral singleton. Used to connect to a device like the LEGO Remote.
pbdrv_bluetooth_peripheral_t peripheral_singleton = {
    .con_handle = NO_CONNECTION,
};

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

PROCESS(pbdrv_bluetooth_spi_process, "Bluetooth SPI");

LIST(task_queue);
static bool bluetooth_ready;
static pbdrv_bluetooth_on_event_t bluetooth_on_event;
static pbdrv_bluetooth_receive_handler_t receive_handler;
static const pbdrv_bluetooth_stm32_cc2640_platform_data_t *pdata = &pbdrv_bluetooth_stm32_cc2640_platform_data;

// HACK: restart observing and scanning repeatedly to work around a bug in the
// Bluetooth chip firmware that causes it to stop receiving advertisements
// after a while.
#define SCAN_RESTART_INTERVAL 3000
#define OBSERVE_RESTART_INTERVAL 10000
static struct etimer scan_restart_timer;
static struct etimer observe_restart_timer;
static bool observe_restart_enabled;

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
        case bleNotConnected:
            return PBIO_ERROR_NO_DEV;
        case bleTimeout:
            return PBIO_ERROR_TIMEDOUT;
        default:
            return PBIO_ERROR_FAILED;
    }
}

static void start_task(pbio_task_t *task, pbio_task_thread_t thread, void *context) {
    pbio_task_init(task, thread, context);
    list_add(task_queue, task);
    process_poll(&pbdrv_bluetooth_spi_process);
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

// Internal Bluetooth driver API implementation

void pbdrv_bluetooth_init(void) {
    pbdrv_gpio_set_pull(&pdata->reset_gpio, PBDRV_GPIO_PULL_NONE);
    bluetooth_reset(RESET_STATE_OUT_LOW);

    pbdrv_gpio_set_pull(&pdata->mrdy_gpio, PBDRV_GPIO_PULL_NONE);
    spi_set_mrdy(false);

    pdata->spi_init();
}

// Public Bluetooth driver API implementation

void pbdrv_bluetooth_power_on(bool on) {
    if (on) {
        process_start(&pbdrv_bluetooth_spi_process);
    } else {
        // REVISIT: should probably gracefully shutdown in case we are in the
        // middle of something
        process_exit(&pbdrv_bluetooth_spi_process);
    }
}

bool pbdrv_bluetooth_is_ready(void) {
    return bluetooth_ready;
}

const char *pbdrv_bluetooth_get_hub_name(void) {
    return pbdrv_bluetooth_hub_name;
}

const char *pbdrv_bluetooth_get_fw_version(void) {
    return pbdrv_bluetooth_fw_version;
}

static PT_THREAD(noop_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);
    task->status = PBIO_SUCCESS;
    PT_END(pt);
}

void pbdrv_bluetooth_queue_noop(pbio_task_t *task) {
    start_task(task, noop_task, NULL);
}

/**
 * Sets advertising data and enables advertisements.
 */
static PT_THREAD(set_discoverable(struct pt *pt, pbio_task_t *task)) {
    uint8_t data[31];

    PT_BEGIN(pt);

    static int8_t tx_power;
    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_LE_readAdvertisingChannelTxPower();
    PT_WAIT_UNTIL(pt, hci_command_complete);

    {
        HCI_StatusCodes_t status = read_buf[9];
        if (status == bleSUCCESS) {
            tx_power = read_buf[10];
        }
    }

    // Set advertising data

    PT_WAIT_WHILE(pt, write_xfer_size);
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
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // Set scan response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    data[0] = sizeof(PNP_ID);  // same as 1 + strlen(PNP_ID)
    data[1] = GAP_ADTYPE_SERVICE_DATA;
    memcpy(&data[2], PNP_ID, sizeof(PNP_ID));
    uint8_t hub_name_len = strlen(pbdrv_bluetooth_hub_name);
    data[11] = hub_name_len + 1;
    data[12] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&data[13], pbdrv_bluetooth_hub_name, hub_name_len);
    _Static_assert(13 + sizeof(pbdrv_bluetooth_hub_name) - 1 <= 31, "scan response is 31 octet max");

    GAP_updateAdvertisingData(GAP_AD_TYPE_SCAN_RSP_DATA, 13 + hub_name_len, data);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // make discoverable

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_makeDiscoverable(ADV_IND, GAP_INITIATOR_ADDR_TYPE_PRIVATE_NON_RESOLVE, NULL,
        GAP_CHANNEL_MAP_ALL, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_start_advertising(void) {
    static pbio_task_t task;
    start_task(&task, set_discoverable, NULL);
}

static PT_THREAD(set_non_discoverable(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_endDiscoverable();
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // REVISIT: technically, this isn't complete until GAP_EndDiscoverableDone
    // event is received

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_stop_advertising(void) {
    static pbio_task_t task;
    start_task(&task, set_non_discoverable, NULL);
}

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    if (connection == PBDRV_BLUETOOTH_CONNECTION_LE && conn_handle != NO_CONNECTION && !busy_disconnecting) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS && pybricks_notify_en) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_UART && uart_tx_notify_en) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL && peripheral_singleton.con_handle != NO_CONNECTION) {
        return true;
    }

    return false;
}

void pbdrv_bluetooth_set_on_event(pbdrv_bluetooth_on_event_t on_event) {
    bluetooth_on_event = on_event;
}

/**
 * Handles sending data via a characteristic value notification.
 */
static PT_THREAD(send_value_notification(struct pt *pt, pbio_task_t *task))
{
    pbdrv_bluetooth_send_context_t *send = task->context;

    PT_BEGIN(pt);

retry:
    PT_WAIT_WHILE(pt, write_xfer_size);

    uint16_t attr_handle;
    if (send->connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS) {
        if (!pybricks_notify_en) {
            task->status = PBIO_ERROR_INVALID_OP;
            goto done;
        }
        attr_handle = pybricks_command_event_char_handle;
    } else if (send->connection == PBDRV_BLUETOOTH_CONNECTION_UART) {
        if (!uart_tx_notify_en) {
            task->status = PBIO_ERROR_INVALID_OP;
            goto done;
        }
        attr_handle = uart_tx_char_handle;
    } else {
        // called with invalid connection
        assert(0);
        task->status = PBIO_ERROR_INVALID_ARG;
        goto done;
    }

    {
        attHandleValueNoti_t req;

        req.handle = attr_handle;
        req.len = send->size;
        req.pValue = send->data;
        ATT_HandleValueNoti(conn_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    HCI_StatusCodes_t status = read_buf[8];

    if (status == blePending) {
        if (task->cancel) {
            task->status = PBIO_ERROR_CANCELED;
            goto done;
        }

        goto retry;
    }

    task->status = PBIO_SUCCESS;

done:
    send->done();

    PT_END(pt);
}

void pbdrv_bluetooth_send(pbdrv_bluetooth_send_context_t *context) {
    static pbio_task_t task;
    start_task(&task, send_value_notification, context);
}

void pbdrv_bluetooth_set_receive_handler(pbdrv_bluetooth_receive_handler_t handler) {
    receive_handler = handler;
}

static PT_THREAD(peripheral_scan_and_connect_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    static uint8_t buf[1];

    assert(peri->match_adv);
    assert(peri->match_adv_rsp);
    assert(peri->notification_handler);

    static pbio_error_t connection_error;

    PT_BEGIN(pt);

    // temporarily stop observing so we can active scan
    if (is_observing) {
        observe_restart_enabled = false;

        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_DeviceDiscoveryCancel();
        PT_WAIT_UNTIL(pt, hci_command_status);

        if (read_buf[8] == bleSUCCESS) {
            PT_WAIT_UNTIL(pt, device_discovery_done);
        }
    }

    // Optionally, disconnect from host (usually Pybricks Code).
    if (conn_handle != NO_CONNECTION &&
        (peri->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_DISCONNECT_HOST)) {
        DEBUG_PRINT_PT(pt, "Disconnect from Pybricks code (%d).\n", conn_handle);
        // Guard used in pbdrv_bluetooth_is_connected so higher level processes
        // won't try to send anything while we are disconnecting.
        busy_disconnecting = true;
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_TerminateLinkReq(conn_handle, 0x13);
        PT_WAIT_UNTIL(pt, conn_handle == NO_CONNECTION);
        busy_disconnecting = false;
    }

restart_scan:

    PROCESS_CONTEXT_BEGIN(&pbdrv_bluetooth_spi_process);
    etimer_set(&scan_restart_timer, SCAN_RESTART_INTERVAL);
    PROCESS_CONTEXT_END(&pbdrv_bluetooth_spi_process);

    // start scanning
    DEBUG_PRINT_PT(pt, "Start scanning.\n");
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_DeviceDiscoveryRequest(GAP_DEVICE_DISCOVERY_MODE_ALL, 1, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PT_WAIT_UNTIL(pt, hci_command_status);

    peri->status = read_buf[8]; // debug

    if (peri->status != bleSUCCESS) {
        task->status = ble_error_to_pbio_error(peri->status);
        goto out;
    }

    device_discovery_done = false;

try_again:

    // Wait for advertising data packet.
    for (;;) {
        advertising_data_received = false;
        PT_WAIT_UNTIL(pt, ({
            if (task->cancel) {
                goto cancel_discovery;
            }
            advertising_data_received || etimer_expired(&scan_restart_timer);
        }));

        // Sub-scan interval timed out, so stop scan and restart.
        if (!advertising_data_received) {
            PT_WAIT_WHILE(pt, write_xfer_size);
            GAP_DeviceDiscoveryCancel();
            PT_WAIT_UNTIL(pt, hci_command_status);
            PT_WAIT_UNTIL(pt, device_discovery_done);
            DEBUG_PRINT_PT(pt, "Sub-scan interval timed out.\n");
            goto restart_scan;
        }


        // Context specific advertisement filter.
        pbdrv_bluetooth_ad_match_result_flags_t adv_flags = peri->match_adv(read_buf[9], &read_buf[19], NULL, &read_buf[11], peri->bdaddr);

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
        PT_WAIT_UNTIL(pt, ({
            if (task->cancel) {
                goto cancel_discovery;
            }
            advertising_data_received || etimer_expired(&scan_restart_timer);
        }));

        // Sub-scan interval timed out, so stop scan and restart.
        if (!advertising_data_received) {
            PT_WAIT_WHILE(pt, write_xfer_size);
            GAP_DeviceDiscoveryCancel();
            PT_WAIT_UNTIL(pt, hci_command_status);
            PT_WAIT_UNTIL(pt, device_discovery_done);
            DEBUG_PRINT_PT(pt, "Scan response timed out.\n");
            goto restart_scan;
        }

        const char *detected_name = (const char *)&read_buf[21];
        const uint8_t *response_address = &read_buf[11];
        pbdrv_bluetooth_ad_match_result_flags_t rsp_flags = peri->match_adv_rsp(read_buf[9], NULL, detected_name, response_address, peri->bdaddr);

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
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_DeviceDiscoveryCancel();
    PT_WAIT_UNTIL(pt, hci_command_status);
    PT_WAIT_UNTIL(pt, device_discovery_done);

    // Connect to the peripheral.

    assert(peri->con_handle == NO_CONNECTION);
    bond_auth_err = NO_AUTH;
    connection_error = PBIO_SUCCESS;
    DEBUG_PRINT_PT(pt, "Found %s. Going to connect.\n", peri->name);

    // Configure to initiate pairing right after connect if bonding required.
    // NB: We must unset "initiate" before we allow a new connection to
    // Pybricks Code or it will try to pair with the PC. However, this happens
    // automatically since gap_init runs again on disconnect, which resets it.
    // It won't unset automatically if Pybricks Code is already connected, but
    // then it doesn't matter since we're already connected.
    PT_WAIT_WHILE(pt, write_xfer_size);
    buf[0] = (peri->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR) ?
        GAPBOND_PAIRING_MODE_INITIATE : GAPBOND_PAIRING_MODE_NO_PAIRING;
    GAP_BondMgrSetParameter(GAPBOND_PAIRING_MODE, 1, buf);
    PT_WAIT_UNTIL(pt, hci_command_status);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_EstablishLinkReq(0, 0, peri->bdaddr_type, peri->bdaddr);
    PT_WAIT_UNTIL(pt, hci_command_status);

    peri->status = read_buf[8]; // debug

    PT_WAIT_UNTIL(pt, ({
        if (task->cancel) {
            connection_error = PBIO_ERROR_CANCELED;
            goto cancel_connect;
        }
        peri->con_handle != NO_CONNECTION;
    }));

    DEBUG_PRINT_PT(pt, "Connected.\n");

    if (peri->options & PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR) {
        PT_WAIT_UNTIL(pt, ({
            if (task->cancel) {
                connection_error = PBIO_ERROR_CANCELED;
                goto cancel_auth_then_disconnect;
            }
            bond_auth_err != NO_AUTH;
        }));
        DEBUG_PRINT_PT(pt, "Auth complete: 0x%02x\n", bond_auth_err);

        if (bond_auth_err != 0) {
            if (bond_auth_err == bleInvalidEventId) {
                // Pairing rejected by peripheral. This can happen if the
                // peripheral has been connected with a different device before
                // we had a chance to delete our bonding info, which can only
                // be done after all connections are closed, including Pybricks Code.
                connection_error = PBIO_ERROR_INVALID_OP; // maps to permission error.
            } else if (bond_auth_err == bleTimeout) {
                // Pairing failed eventually, usually because it got no replies
                // from peripheral at all while connected to Pybricks Code.
                connection_error = PBIO_ERROR_TIMEDOUT;
            } else {
                connection_error = PBIO_ERROR_FAILED;
            }
            goto disconnect;
        }
    }

    task->status = PBIO_SUCCESS;

    goto out;

cancel_auth_then_disconnect:

    DEBUG_PRINT_PT(pt, "Cancel auth.\n");
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_TerminateAuth(peri->con_handle, 0x13);
    PT_WAIT_UNTIL(pt, hci_command_status);

disconnect:

    DEBUG_PRINT_PT(pt, "Disconnect due to %s.\n", task->cancel ? "cancel" : "error");
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_TerminateLinkReq(peri->con_handle, 0x13);
    PT_WAIT_UNTIL(pt, hci_command_status);
    task->status = connection_error;
    goto out;

cancel_connect:
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_TerminateLinkReq(0xFFFE, 0x13);
    PT_WAIT_UNTIL(pt, hci_command_status);

    goto end_cancel;

cancel_discovery:
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_DeviceDiscoveryCancel();
    PT_WAIT_UNTIL(pt, hci_command_status);

    PT_WAIT_UNTIL(pt, device_discovery_done);

end_cancel:
    task->status = PBIO_ERROR_CANCELED;

out:
    // start passive scanning again if observing
    if (is_observing) {
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_DeviceDiscoveryRequest(GAP_DEVICE_DISCOVERY_MODE_NONDISCOVERABLE, 0, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
        PT_WAIT_UNTIL(pt, hci_command_status);

        if (read_buf[8] != bleSUCCESS) {
            task->status = ble_error_to_pbio_error(read_buf[8]);
            PT_EXIT(pt);
        }

        device_discovery_done = false;

        PROCESS_CONTEXT_BEGIN(&pbdrv_bluetooth_spi_process);
        etimer_set(&observe_restart_timer, OBSERVE_RESTART_INTERVAL);
        PROCESS_CONTEXT_END(&pbdrv_bluetooth_spi_process);
        observe_restart_enabled = true;
    }

    PT_END(pt);
}

static PT_THREAD(periperal_discover_characteristic_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
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
    PT_WAIT_UNTIL(pt, hci_command_status);

    peri->status = read_buf[8]; // debug

    // GATT_DiscCharsByUUID() sends a "read by type request" so we have to
    // wait until we get an error response to the request or we get a "read
    // by type response" with status of bleProcedureComplete. There can be
    // multiple responses received before the procedure is complete.
    // REVISIT: what happens when remote is disconnected while waiting here?

    PT_WAIT_UNTIL(pt, ({
        uint8_t *payload;
        uint8_t len;
        uint16_t event;
        HCI_StatusCodes_t status;
        (payload = get_vendor_event(peri->con_handle, &event, &status, &len)) && ({
            if (event == ATT_EVENT_ERROR_RSP && payload[0] == ATT_READ_BY_TYPE_REQ) {
                task->status = PBIO_ERROR_FAILED;
                goto disconnect;
            }

            event == ATT_EVENT_READ_BY_TYPE_RSP;
        }) && ({
            // hopefully the docs are correct and this is the only possible error
            if (status == bleTimeout) {
                task->status = PBIO_ERROR_TIMEDOUT;
                goto disconnect;
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
        task->status = PBIO_SUCCESS;
        PT_EXIT(pt);
    }

    // enable notifications

retry:
    PT_WAIT_WHILE(pt, write_xfer_size);
    {
        static const uint16_t enable = 0x0001;
        attWriteReq_t req = {
            // assuming that client characteristic configuration descriptor
            // is next attribute after the characteristic value attribute
            .handle = peri->char_now->handle + 1,
            .len = sizeof(enable),
            .pValue = (uint8_t *)&enable,
        };
        // REVISIT: we may want to change this to write with response to ensure
        // that the remote device received the message.
        GATT_WriteNoRsp(peri->con_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    HCI_StatusCodes_t status = read_buf[8];

    if (status == blePending) {
        if (task->cancel) {
            task->status = PBIO_ERROR_CANCELED;
            goto disconnect;
        }

        goto retry;
    }

    peri->status = read_buf[8]; // debug
    task->status = PBIO_SUCCESS;

    PT_EXIT(pt);

disconnect:
    // task->status must be set before goto disconnect.
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_TerminateLinkReq(peri->con_handle, 0x13);
    PT_WAIT_UNTIL(pt, hci_command_status);

    PT_END(pt);
}

void pbdrv_bluetooth_peripheral_scan_and_connect(pbio_task_t *task, pbdrv_bluetooth_ad_match_t match_adv, pbdrv_bluetooth_ad_match_t match_adv_rsp, pbdrv_bluetooth_receive_handler_t notification_handler, pbdrv_bluetooth_peripheral_options_t options) {
    // Unset previous bluetooth addresses and other state variables.
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    memset(peri, 0, sizeof(pbdrv_bluetooth_peripheral_t));
    peri->con_handle = NO_CONNECTION;

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

    static uint8_t status;

retry:
    DEBUG_PRINT_PT(pt, "going to read %04x:\n", peri->char_now->handle);
    PT_WAIT_WHILE(pt, write_xfer_size);
    {
        attReadReq_t req = {
            .handle = peri->char_now->handle,
        };
        GATT_ReadCharValue(peri->con_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    status = read_buf[8];

    if (status != bleSUCCESS) {
        if (status == blePending) {
            if (task->cancel) {
                goto cancel;
            }
            goto retry;
        }
        goto exit;
    }

    // This gets a bit tricky. Once the request has been sent, we can't cancel
    // the task, so we have to wait for the response (otherwise the response
    // could be confused with the next request). The device could also become
    // disconnected, in which case we never receive a response.
    PT_WAIT_UNTIL(pt, ({
        if (peri->con_handle == NO_CONNECTION) {
            task->status = PBIO_ERROR_NO_DEV;
            PT_EXIT(pt);
        }

        uint8_t *payload;
        uint8_t len;
        uint16_t event;
        (payload = get_vendor_event(peri->con_handle, &event, &status, &len)) && ({
            if (event == ATT_EVENT_ERROR_RSP && payload[0] == ATT_READ_REQ
                && pbio_get_uint16_le(&payload[1]) == peri->char_now->handle) {
                task->status = PBIO_ERROR_FAILED;
                PT_EXIT(pt);
            }
            // Get the response value.
            if (event == ATT_EVENT_READRSP && len <= sizeof(peri->char_now->value)) {
                peri->char_now->value_len = len;
                memcpy(peri->char_now->value, payload, len);
            }
            event == ATT_EVENT_READRSP;
        });
    }));

    DEBUG_PRINT_PT(pt, "Read %04x with status %d\n", peri->char_now->handle, status);

exit:
    task->status = ble_error_to_pbio_error(status);
    PT_EXIT(pt);

cancel:
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

retry:
    PT_WAIT_WHILE(pt, write_xfer_size);
    {
        GattWriteCharValue_t req = {
            .connHandle = peri->con_handle,
            .handle = pbio_get_uint16_le(value->handle),
            .value = value->data,
            .dataSize = value->size,
        };
        GATT_WriteCharValue(&req);
    }

    PT_WAIT_UNTIL(pt, hci_command_status);

    HCI_StatusCodes_t status = read_buf[8];

    if (status != bleSUCCESS) {
        if (status == blePending) {
            if (task->cancel) {
                goto cancel;
            }

            goto retry;
        }

        goto exit;
    }

    // This gets a bit tricky. Once the request has been sent, we can't cancel
    // the task, so we have to wait for the response (otherwise the response
    // could be confused with the next request). The device could also become
    // disconnected, in which case we never receive a response.

    PT_WAIT_UNTIL(pt, ({
        if (peri->con_handle == NO_CONNECTION) {
            task->status = PBIO_ERROR_NO_DEV;
            PT_EXIT(pt);
        }

        uint8_t *payload;
        uint8_t len;
        uint16_t event;
        (payload = get_vendor_event(peri->con_handle, &event, &status, &len)) && ({
            if (event == ATT_EVENT_ERROR_RSP && payload[0] == ATT_WRITE_REQ
                && pbio_get_uint16_le(&payload[1]) == pbio_get_uint16_le(value->handle)) {

                task->status = PBIO_ERROR_FAILED;
                PT_EXIT(pt);
            }

            event == ATT_EVENT_WRITE_RSP;
        });
    }));

exit:
    task->status = ble_error_to_pbio_error(status);

    PT_EXIT(pt);

cancel:
    task->status = PBIO_ERROR_CANCELED;

    PT_END(pt);
}

void pbdrv_bluetooth_peripheral_write(pbio_task_t *task, pbdrv_bluetooth_value_t *value) {
    start_task(task, peripheral_write_task, value);
}

static PT_THREAD(peripheral_disconnect_task(struct pt *pt, pbio_task_t *task)) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PT_BEGIN(pt);

    if (peri->con_handle != NO_CONNECTION) {
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_TerminateLinkReq(peri->con_handle, 0x13);
        PT_WAIT_UNTIL(pt, hci_command_status);
        PT_WAIT_UNTIL(pt, peri->con_handle == NO_CONNECTION);
    }

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_peripheral_disconnect(pbio_task_t *task) {
    start_task(task, peripheral_disconnect_task, NULL);
}

static PT_THREAD(broadcast_task(struct pt *pt, pbio_task_t *task)) {
    pbdrv_bluetooth_value_t *value = task->context;

    PT_BEGIN(pt);

    if (value->size > B_MAX_ADV_LEN) {
        task->status = PBIO_ERROR_INVALID_ARG;
        PT_EXIT(pt);
    }

    // HACK: calling GAP_updateAdvertisingData() repeatedly will cause the
    // Bluetooth chips on Technic and City hubs to eventually lock up. So we
    // call the standard Bluetooth command instead. We still get the vendor-
    // specific command complete event as if we had called the TI command (but
    // not the command status).
    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_LE_setAdvertisingData(value->size, value->data);
    PT_WAIT_UNTIL(pt, hci_command_complete);

    if (!is_broadcasting) {
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_makeDiscoverable(
            ADV_IND,
            GAP_INITIATOR_ADDR_TYPE_PRIVATE_NON_RESOLVE, NULL,
            GAP_CHANNEL_MAP_ALL, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_WHITELIST);
        PT_WAIT_UNTIL(pt, hci_command_status);

        if (read_buf[8] != bleSUCCESS) {
            task->status = ble_error_to_pbio_error(read_buf[8]);
            PT_EXIT(pt);
        }

        // wait for make discoverable done event
        PT_WAIT_UNTIL(pt, hci_command_complete);

        is_broadcasting = true;
    }

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_start_broadcasting(pbio_task_t *task, pbdrv_bluetooth_value_t *value) {
    start_task(task, broadcast_task, value);
}

static PT_THREAD(stop_broadcast_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    if (is_broadcasting) {
        // TODO: resolve city hub broadcast issues
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_endDiscoverable();
        PT_WAIT_UNTIL(pt, hci_command_status);

        if (read_buf[8] == bleSUCCESS) {
            // wait for discovery cancel done event
            PT_WAIT_UNTIL(pt, hci_command_complete);
        }

        // Status could also be bleIncorrectMode which means "Not advertising".
        // This is not expected, but should be safe to ignore.

        is_broadcasting = false;
    }

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_stop_broadcasting(pbio_task_t *task) {
    start_task(task, stop_broadcast_task, NULL);
}

static PT_THREAD(observe_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    if (!is_observing) {
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_DeviceDiscoveryRequest(GAP_DEVICE_DISCOVERY_MODE_NONDISCOVERABLE, 0, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
        PT_WAIT_UNTIL(pt, hci_command_status);

        if (read_buf[8] != bleSUCCESS) {
            task->status = ble_error_to_pbio_error(read_buf[8]);
            PT_EXIT(pt);
        }

        device_discovery_done = false;
        is_observing = true;

        PROCESS_CONTEXT_BEGIN(&pbdrv_bluetooth_spi_process);
        etimer_set(&observe_restart_timer, OBSERVE_RESTART_INTERVAL);
        PROCESS_CONTEXT_END(&pbdrv_bluetooth_spi_process);
        observe_restart_enabled = true;
    }

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_start_observing(pbio_task_t *task, pbdrv_bluetooth_start_observing_callback_t callback) {
    observe_callback = callback;
    start_task(task, observe_task, NULL);
}

static PT_THREAD(stop_observe_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    if (is_observing) {
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_DeviceDiscoveryCancel();
        PT_WAIT_UNTIL(pt, hci_command_status);

        if (read_buf[8] == bleSUCCESS) {
            PT_WAIT_UNTIL(pt, device_discovery_done);
        }

        is_observing = false;
    }

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_stop_observing(pbio_task_t *task) {
    observe_callback = NULL;
    // avoid restarting observing even if this task get queued
    observe_restart_enabled = false;

    start_task(task, stop_observe_task, NULL);
}

// Driver interrupt callbacks

void pbdrv_bluetooth_stm32_cc2640_srdy_irq(bool srdy) {
    spi_srdy = srdy;

    if (!srdy) {
        spi_n_srdy_count++;
    }

    process_poll(&pbdrv_bluetooth_spi_process);
}

void pbdrv_bluetooth_stm32_cc2640_spi_xfer_irq(void) {
    spi_xfer_complete = true;
    process_poll(&pbdrv_bluetooth_spi_process);
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
                        pbio_pybricks_hub_capabilities(buf, conn_mtu - 3, PBSYS_APP_HUB_FEATURE_FLAGS, PBSYS_STORAGE_MAX_PROGRAM_SIZE);
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
                        if (receive_handler) {
                            err = receive_handler(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS, &data[10], pdu_len - 4);
                        }
                    } else if (char_handle == pybricks_command_event_char_handle + 1) {
                        pybricks_notify_en = data[10];
                        err = PBIO_PYBRICKS_ERROR_OK;
                        DBG("noti: %d", pybricks_notify_en);
                    } else if (char_handle == uart_rx_char_handle) {
                        if (receive_handler) {
                            err = receive_handler(PBDRV_BLUETOOTH_CONNECTION_UART, &data[10], pdu_len - 4);
                        }
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
                    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
                    if (peri->notification_handler) {
                        peri->notification_handler(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL, &data[8], pdu_len - 2);
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
                    if (observe_callback) {
                        uint8_t event_type = data[3];
                        // uint8_t addr_type = data[4];
                        // uint8_t *addr = &data[5];
                        int8_t rssi = data[11];
                        uint8_t data_len = data[12];
                        uint8_t *data_field = &data[13];

                        observe_callback(event_type, data_field, data_len, rssi);
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

    if (bluetooth_on_event) {
        bluetooth_on_event();
    }
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

static PT_THREAD(gatt_init(struct pt *pt)) {
    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 1, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    gatt_service_handle = (read_buf[13] << 8) | read_buf[12];
    gatt_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    DBG("gatt: %04X", gatt_service_handle);

    /**************************************************************************/

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 7, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(DEVICE_NAME_UUID, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(APPEARANCE_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(PERI_CONN_PARAM_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    gap_service_handle = (read_buf[13] << 8) | read_buf[12];
    gap_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    DBG("gap: %04X", gap_service_handle);

    PT_END(pt);
}

static PT_THREAD(gap_init(struct pt *pt)) {

    static uint8_t buf[1];

    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    buf[0] = GAPBOND_PAIRING_MODE_NO_PAIRING;
    GAP_BondMgrSetParameter(GAPBOND_PAIRING_MODE, 1, buf);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // Read the number of bonds stored in flash.
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_BondMgrGetParameter(GAPBOND_BOND_COUNT);
    PT_WAIT_UNTIL(pt, hci_command_status);

    // Connecting to previously bonded devices does not always work reliably
    // due to particular behaviors of some peripherals, combined with the
    // random address used by Pybricks. Bonding takes only a few seconds, so we
    // just always clear the bond information if there is any, and start over.
    if (read_buf[12] > 0) {
        DEBUG_PRINT_PT(pt, "Old bond count: %d\n", read_buf[12]);

        // Erase all bonds stored on Bluetooth chip. We can also erase local
        // info, but this does not appear to be necessary.
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_BondMgrSetParameter(GAPBOND_ERASE_ALLBONDS, 0, NULL);
        PT_WAIT_UNTIL(pt, hci_command_status);

        // Read the number of bonds stored in flash, should now be zero.
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_BondMgrGetParameter(GAPBOND_BOND_COUNT);
        PT_WAIT_UNTIL(pt, hci_command_status);
        DEBUG_PRINT_PT(pt, "New bond count: %d\n", read_buf[12]);
    }

    PT_WAIT_WHILE(pt, write_xfer_size);
    buf[0] = 0; // disabled
    GAP_BondMgrSetParameter(GAPBOND_MITM_PROTECTION, 1, buf);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    buf[0] = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
    GAP_BondMgrSetParameter(GAPBOND_IO_CAPABILITIES, 1, buf);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    buf[0] = 1; // enabled, as in allowed. It won't happen by default.
    GAP_BondMgrSetParameter(GAPBOND_BONDING_ENABLED, 1, buf);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    buf[0] = GAPBOND_SECURE_CONNECTION_NONE;
    GAP_BondMgrSetParameter(GAPBOND_SECURE_CONNECTION, 1, buf);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // Set scan timeout on btchip to infinite. Separate timeout
    // is implemented to stop program and thus scan
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_GEN_DISC_SCAN, 0);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // Set scan timeout on btchip to infinite. Separate timeout
    // is implemented to stop program and thus scan
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_LIM_DISC_SCAN, 0);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, 40);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, 40);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_ADV_INT_MIN, 40);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_ADV_INT_MAX, 40);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // scan interval general discovery: 48 * 0.625ms = 30ms
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_GEN_DISC_SCAN_INT, 48);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // scan window general discovery: 48 * 0.625ms = 30ms
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_GEN_DISC_SCAN_WIND, 48);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MIN, 40);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MAX, 40);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // scan interval connection established: 48 * 0.625ms = 30ms
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_SCAN_INT, 48);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // scan window connection established: 48 * 0.625ms = 30ms
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_SCAN_WIND, 48);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, 60);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // When acting as a central, make the hub reject connection parameter
    // changes requested by peripherals. This ensures that things like
    // broadcasting keep working. May need to revisit this if we ever have
    // peripherals that only work with their own connection parameters.
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_REJECT_CONN_PARAMS, 1);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_LATENCY, 0);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_MIN_CE_LEN, 4);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_MAX_CE_LEN, 4);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_FILTER_ADV_REPORTS, 0);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_deviceInit(GAP_PROFILE_PERIPHERAL | GAP_PROFILE_CENTRAL, 8, NULL, NULL, 0);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // This sets the device address to a new random value each time we reset
    // the Bluetooth chip.
    PT_WAIT_WHILE(pt, write_xfer_size);
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
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_END(pt);
}

// Initializes the Bluetooth chip
// this function is largely inspired by the LEGO bootloader
static PT_THREAD(hci_init(struct pt *pt)) {
    PT_BEGIN(pt);

    // // set the Bluetooth address

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_EXT_setBdaddr(pdata->bd_addr);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // read Bluetooth address

    // TODO: skip getting bdaddr - it is returned by both the previous and next
    // commands anyway

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_readBdaddr();
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // set Tx power level

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_EXT_setTxPower(HCI_EXT_CC26XX_TX_POWER_0_DBM);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_readLocalVersionInfo();
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // firmware version is TI BLE-Stack SDK version
    snprintf(pbdrv_bluetooth_fw_version, sizeof(pbdrv_bluetooth_fw_version),
        "v%u.%02u.%02u", read_buf[12], read_buf[11] >> 4, read_buf[11] & 0xf);

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_EXT_setLocalSupportedFeatures(HCI_EXT_LOCAL_FEATURE_ENCRYTION);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    Util_readLegoHwVersion();
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // REVISIT: LEGO Technic hub firmware uses this as hub HW version

    PT_WAIT_WHILE(pt, write_xfer_size);
    Util_readLegoFwVersion();
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // REVISIT: LEGO Technic hub firmware appends this to radio FW version

    PT_END(pt);
}

static PT_THREAD(init_device_information_service(struct pt *pt)) {
    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 7, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(FIRMWARE_REVISION_STRING_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(SOFTWARE_REVISION_STRING_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(PNP_ID_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    dev_info_service_handle = (read_buf[13] << 8) | read_buf[12];
    dev_info_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    DBG("device information: %04X", dev_info_service_handle);

    PT_END(pt);
}

static PT_THREAD(init_pybricks_service(struct pt *pt)) {
    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 6, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(pbio_pybricks_command_event_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CLIENT_CHAR_CFG_UUID, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(pbio_pybricks_hub_capabilities_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    pybricks_service_handle = (read_buf[13] << 8) | read_buf[12];
    pybricks_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    pybricks_command_event_char_handle = pybricks_service_handle + 2;
    pybricks_capabilities_char_handle = pybricks_service_handle + 4;
    DBG("pybricks: %04X", pybricks_service_handle);

    PT_END(pt);
}

static PT_THREAD(init_uart_service(struct pt *pt)) {
    PT_BEGIN(pt);

    // add the Nordic UART service (inspired by Add_Sample_Service() from
    // sample_service.c in BlueNRG vendor sample code and Adafruit config file
    // https://github.com/adafruit/Adafruit_nRF8001/blob/master/utility/uart/UART_over_BLE.xml)

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 6, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(pbio_nus_rx_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(pbio_nus_tx_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CLIENT_CHAR_CFG_UUID, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    uart_service_handle = (read_buf[13] << 8) | read_buf[12];
    uart_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    uart_rx_char_handle = uart_service_handle + 2;
    uart_tx_char_handle = uart_service_handle + 4;
    DBG("uart: %04X", uart_service_handle);

    PT_END(pt);
}

static PT_THREAD(init_task(struct pt *pt, pbio_task_t *task)) {
    static struct pt child_pt;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child_pt, hci_init(&child_pt));
    PT_SPAWN(pt, &child_pt, gatt_init(&child_pt));
    PT_SPAWN(pt, &child_pt, gap_init(&child_pt));
    PT_SPAWN(pt, &child_pt, init_device_information_service(&child_pt));
    PT_SPAWN(pt, &child_pt, init_pybricks_service(&child_pt));
    PT_SPAWN(pt, &child_pt, init_uart_service(&child_pt));
    bluetooth_ready = true;
    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

PROCESS_THREAD(pbdrv_bluetooth_spi_process, ev, data) {
    static struct etimer timer;
    static uint8_t read_xfer_size, xfer_size;

    PROCESS_EXITHANDLER({
        spi_set_mrdy(false);
        bluetooth_reset(RESET_STATE_OUT_LOW);
        bluetooth_ready = pybricks_notify_en = uart_tx_notify_en =
            is_broadcasting = is_observing = observe_restart_enabled = false;
        conn_handle = peripheral_singleton.con_handle = NO_CONNECTION;

        pbio_task_t *task;
        while ((task = list_pop(task_queue)) != NULL) {
            if (task->status == PBIO_ERROR_AGAIN) {
                task->status = PBIO_ERROR_CANCELED;
                // REVISIT: some tasks have done() callback that probably needs
                // to be called here
            }
        }

        PROCESS_EXIT();
    });

    PROCESS_BEGIN();

start:
    // take Bluetooth chip out of reset
    bluetooth_reset(RESET_STATE_OUT_HIGH);

    // not sure why we need this
    etimer_set(&timer, 150);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
    bluetooth_reset(RESET_STATE_INPUT);

    // bluetooth chip should not have any messages for us yet, so spi_srdy
    // should be false, if not wait a while and try resetting the chip
    if (spi_srdy) {
        static bool timed_out = false;

        etimer_set(&timer, 500);

        while (spi_srdy) {
            PROCESS_WAIT_EVENT();
            if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
                timed_out = true;
            }
        }

        if (timed_out) {
            etimer_set(&timer, 3000);
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
            bluetooth_reset(RESET_STATE_OUT_LOW);
            etimer_set(&timer, 150);
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
            goto start;
        }
    }

    etimer_set(&timer, 100);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

    static pbio_task_t task;
    start_task(&task, init_task, NULL);

    for (;;) {
        PROCESS_WAIT_UNTIL({
            for (;;) {
                if (observe_restart_enabled && etimer_expired(&observe_restart_timer)) {
                    static pbio_task_t observe_restart_stop_task;
                    static pbio_task_t observe_restart_start_task;
                    pbdrv_bluetooth_start_observing_callback_t callback = observe_callback;

                    pbdrv_bluetooth_stop_observing(&observe_restart_stop_task);
                    pbdrv_bluetooth_start_observing(&observe_restart_start_task, callback);
                }

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
            PROCESS_WAIT_UNTIL(spi_srdy);
        } else {
            // if we are reading only, the write buffer has to be all 0s
            memset(write_buf, 0, PBIO_ARRAY_SIZE(write_buf));
            // indicates that write_buf is in use
            write_xfer_size = PBIO_ARRAY_SIZE(write_buf);
        }

        // send the write header
        spi_xfer_complete = false;
        pdata->spi_start_xfer(write_buf, read_buf, NPI_SPI_HEADER_LEN);
        PROCESS_WAIT_UNTIL(spi_xfer_complete);

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
        PROCESS_WAIT_UNTIL(spi_xfer_complete);

        // After the transfer is complete, we release the MRDY signal to the
        // Bluetooth chip and it acknowledges by releasing the SRDY signal.
        static uint8_t prev_n_srdy_count;
        prev_n_srdy_count = spi_n_srdy_count;
        spi_set_mrdy(false);
        // Multiple interrupts can happen between line above and line below,
        // so we can't use PROCESS_WAIT_UNTIL(!spi_srdy) as the SRDY signal may
        // go off and back on again before we can read it once. So we have a
        // separate falling edge trigger to ensure we catch the event.
        PROCESS_WAIT_UNTIL(spi_n_srdy_count != prev_n_srdy_count);

        // set to 0 to indicate that xfer is complete
        write_xfer_size = 0;

        if (read_xfer_size) {
            // handle the received data
            if (read_buf[NPI_SPI_HEADER_LEN] == HCI_EVENT_PACKET) {
                handle_event(&read_buf[NPI_SPI_HEADER_LEN + 1]);
            }
        }
    }

    PROCESS_END();
}

// implements function for bt5stack library
HCI_StatusCodes_t HCI_sendHCICommand(uint16_t opcode, uint8_t *pData, uint8_t dataLength) {
    assert(write_xfer_size == 0);

    write_buf[0] = NPI_SPI_SOF;
    write_buf[1] = dataLength + 4;
    write_buf[2] = HCI_CMD_PACKET;
    pbio_set_uint16_le(&write_buf[3], opcode);
    write_buf[5] = dataLength;
    if (pData) {
        memcpy(&write_buf[6], pData, dataLength);
    }

    // calculate and append FCS byte
    uint8_t checksum = 0;
    for (int i = 1; i < dataLength + 6; i++) {
        checksum ^= write_buf[i];
    }
    write_buf[dataLength + 6] = checksum;

    write_xfer_size = dataLength + 7;

    // NB: some commands only receive CommandStatus, others only receive specific
    // reply, others receive both
    hci_command_opcode = opcode;
    hci_command_status = false;
    hci_command_complete = false;

    return bleSUCCESS;
}

#endif // PBDRV_CONFIG_BLUETOOTH_STM32_CC2640
