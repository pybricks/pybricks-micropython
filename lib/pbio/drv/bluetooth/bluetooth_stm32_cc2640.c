// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// Bluetooth for STM32 MCU with TI CC2640

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_STM32_CC2640

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>
#include <pbio/protocol.h>
#include <pbio/task.h>
#include <pbio/util.h>
#include <pbio/version.h>

#include <contiki.h>
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

#define DEBUG 0
#if DEBUG == 1
#include <stdio.h>
#include <stm32f0xx.h>
#define DBG(fmt, ...) { \
        char dbg[64]; \
        snprintf(dbg, 64, fmt "\r\n",##__VA_ARGS__); \
        for (char *d = dbg; *d; d++) { \
            while (!(USART3->ISR & USART_ISR_TXE)) { } \
            USART3->TDR = *d; \
        } \
}
#elif DEBUG == 2
#include <stdio.h>
#include <stm32l431xx.h>
#define DBG(fmt, ...) { \
        char dbg[64]; \
        snprintf(dbg, 64, fmt "\r\n",##__VA_ARGS__); \
        for (char *d = dbg; *d; d++) { \
            while (!(LPUART1->ISR & USART_ISR_TXE)) { } \
            LPUART1->TDR = *d; \
        } \
}
#else
#define DBG(...)
#endif

// hub name goes in special section so that it can be modified when flashing firmware
__attribute__((section(".name")))
char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

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
// set to false when xfer is started and true when xfer is complete
volatile bool spi_xfer_complete;

// set to the pending hci command opcode when a command is sent
static uint16_t hci_command_opcode;
// set to false when hci command is started and true when command status is received
static bool hci_command_status;
// set to false when hci command is started and true when command is completed
static bool hci_command_complete;
// used to synchronize advertising data handler
static bool advertising_data_received;
// handle to connected Bluetooth device
static uint16_t conn_handle = NO_CONNECTION;
// handle to connected remote control
static uint16_t remote_handle = NO_CONNECTION;
// handle to LWP3 characteristic on remote
static uint16_t remote_lwp3_char_handle = NO_CONNECTION;

// GATT service handles
static uint16_t gatt_service_handle, gatt_service_end_handle;
// GAP service handles
static uint16_t gap_service_handle, gap_service_end_handle;
// Device information service handles
static uint16_t dev_info_service_handle, dev_info_service_end_handle;
// Pybricks service handles
static uint16_t pybricks_service_handle, pybricks_service_end_handle, pybricks_char_handle;
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
static pbdrv_bluetooth_receive_handler_t notification_handler;
static const pbdrv_bluetooth_stm32_cc2640_platform_data_t *pdata = &pbdrv_bluetooth_stm32_cc2640_platform_data;

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

/**
 * Sets advertising data and enables advertisements.
 */
static PT_THREAD(set_discoverable(struct pt *pt, pbio_task_t *task)) {
    uint8_t data[31];

    PT_BEGIN(pt);

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
    data[23] = 0;
    GAP_updateAdvertistigData(GAP_AD_TYPE_ADVERTISEMNT_DATA, 24, data);
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

    GAP_updateAdvertistigData(GAP_AD_TYPE_SCAN_RSP_DATA, 13 + hub_name_len, data);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // make discoverable

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_makeDiscoverable(ADV_IND, GAP_INITIATOR_ADDR_TYPE_PUBLIC, NULL,
        GAP_CHANNEL_MAP_ALL, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_start_advertising(void) {
    static pbio_task_t task;
    pbio_task_init(&task, set_discoverable, NULL);
    pbio_task_start(task_queue, &task);
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
    pbio_task_init(&task, set_non_discoverable, NULL);
    pbio_task_start(task_queue, &task);
}

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    if (connection == PBDRV_BLUETOOTH_CONNECTION_LE && conn_handle != NO_CONNECTION) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS && pybricks_notify_en) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_UART && uart_tx_notify_en) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_HANDSET && remote_handle != NO_CONNECTION) {
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
        attr_handle = pybricks_char_handle;
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
        goto retry;
    }

    task->status = PBIO_SUCCESS;

done:
    send->done();

    PT_END(pt);
}

void pbdrv_bluetooth_send(pbdrv_bluetooth_send_context_t *context) {
    static pbio_task_t task;
    pbio_task_init(&task, send_value_notification, context);
    pbio_task_start(task_queue, &task);
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

    // start scanning
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_DeviceDiscoveryRequest(GAP_DEVICE_DISCOVERY_MODE_ALL, 1, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PT_WAIT_UNTIL(pt, hci_command_status);

    context->status = read_buf[8]; // debug

    for (;;) {
        advertising_data_received = false;
        PT_WAIT_UNTIL(pt, {
            if (task->cancel) {
                goto cancel_discovery;
            }
            advertising_data_received;
        });

        // TODO: Properly parse advertising data. For now, we are assuming that
        // the service UUID is at a fixed position and we are getting only
        // GAP_ADTYPE_128BIT_COMPLETE and not GAP_ADTYPE_128BIT_MORE
        if (
            read_buf[9] != ADV_IND /* connectable undirected advertisement */ ||
            read_buf[22] != 17 /* length */ || read_buf[23] != GAP_ADTYPE_128BIT_COMPLETE ||
            !pbio_uuid128_reverse_compare(&read_buf[24], pbio_lwp3_hub_service_uuid) ||
            read_buf[45] != LWP3_HUB_KIND_HANDSET) {

            // if this is not LEGO Powered Up remote, keep scanning
            continue;
        }

        // save the Bluetooth address for later
        // addr_type = read_buf[10];
        memcpy(context->bdaddr, &read_buf[11], 6);

        // TODO: wait for scan response that matches Bluetooth address for
        // for checking local name
        // read_buf[9] == 4 /* Scan Response */

        break;
    }

    // stop scanning
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_DeviceDiscoveryCancel();
    PT_WAIT_UNTIL(pt, hci_command_status);

    // connect

    assert(remote_handle == NO_CONNECTION);

    PT_WAIT_WHILE(pt, write_xfer_size);
    // REVISIT: might want to store address type from advertising data and pass
    // it in here instead of assuming public address type
    GAP_EstablishLinkReq(0, 0, ADDRTYPE_PUBLIC, context->bdaddr);
    PT_WAIT_UNTIL(pt, hci_command_status);

    context->status = read_buf[8]; // debug

    PT_WAIT_UNTIL(pt, {
        if (task->cancel) {
            goto cancel_connect;
        }
        remote_handle != NO_CONNECTION;
    });

    // discover LWP3 characteristic to get attribute handle

    assert(remote_lwp3_char_handle == NO_CONNECTION);

    PT_WAIT_WHILE(pt, write_xfer_size);
    {
        attReadByTypeReq_t req = {
            .startHandle = 0x0001,
            .endHandle = 0xFFFF,
            .type.len = 16,
        };
        pbio_uuid128_reverse_copy(req.type.uuid, pbio_lwp3_hub_char_uuid);
        GATT_DiscCharsByUUID(remote_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    context->status = read_buf[8]; // debug

    // Assuming that we only ever get successful ATT_ReadByTypeRsp and failed
    // ATT_ReadByTypeRsp or ATT_ErrorRsp.
    PT_WAIT_UNTIL(pt, {
        if (task->cancel) {
            goto cancel_disconnect;
        }
        remote_lwp3_char_handle != NO_CONNECTION;
    });

    // enable notifications

    PT_WAIT_WHILE(pt, write_xfer_size);
    {
        static const uint16_t enable = 0x0001;
        attWriteReq_t req = {
            .handle = remote_lwp3_char_handle + 2,
            .len = sizeof(enable),
            .pValue = (uint8_t *)&enable,
        };
        GATT_WriteNoRsp(remote_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    context->status = read_buf[8]; // debug

    // set mode for left buttons

    PT_WAIT_WHILE(pt, write_xfer_size);
    {
        // 0x0a == length
        // 0x00 == local hub
        // 0x41 == Port Input Format Setup (Single)
        // 0x00 == Port ID - left buttons
        // 0x04 == mode - KEYSD
        // 0x00000001 == delta interval
        // 0x01 == enable notifications
        static const uint8_t command[] = { 0x0a, 0x00, 0x41, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x01 };
        attWriteReq_t req = {
            .handle = remote_lwp3_char_handle + 1,
            .len = sizeof(command),
            .pValue = (uint8_t *)command,
        };
        GATT_WriteNoRsp(remote_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    context->status = read_buf[8]; // debug

    // set mode for right buttons

    PT_WAIT_WHILE(pt, write_xfer_size);
    {
        // 0x0a == length
        // 0x00 == local hub
        // 0x41 == Port Input Format Setup (Single)
        // 0x01 == Port ID - right buttons
        // 0x04 == mode - KEYSD
        // 0x00000001 == delta interval
        // 0x01 == enable notifications
        static const uint8_t command[] = { 0x0a, 0x00, 0x41, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00, 0x01 };
        attWriteReq_t req = {
            .handle = remote_lwp3_char_handle + 1,
            .len = sizeof(command),
            .pValue = (uint8_t *)command,
        };
        GATT_WriteNoRsp(remote_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    context->status = read_buf[8]; // debug

    task->status = PBIO_SUCCESS;
    PT_EXIT(pt);

cancel_disconnect:
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_TerminateLinkReq(remote_handle, 0x13);
    PT_WAIT_UNTIL(pt, hci_command_status);

    goto end_cancel;

cancel_connect:
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_TerminateLinkReq(0xFFFE, 0x13);
    PT_WAIT_UNTIL(pt, hci_command_status);

    goto end_cancel;

cancel_discovery:
    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_DeviceDiscoveryCancel();
    PT_WAIT_UNTIL(pt, hci_command_status);

end_cancel:
    task->status = PBIO_ERROR_CANCELED;
    PT_END(pt);
}

void pbdrv_bluetooth_scan_and_connect(pbio_task_t *task, pbdrv_bluetooth_scan_and_connect_context_t *context) {
    pbio_task_init(task, scan_and_connect_task, context);
    pbio_task_start(task_queue, task);
}

static PT_THREAD(disconnect_remote_task(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    if (remote_handle != NO_CONNECTION) {
        PT_WAIT_WHILE(pt, write_xfer_size);
        GAP_TerminateLinkReq(remote_handle, 0x13);
        PT_WAIT_UNTIL(pt, hci_command_status);
    }

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_disconnect_remote(void) {
    static pbio_task_t task;
    pbio_task_init(&task, disconnect_remote_task, NULL);
    pbio_task_start(task_queue, &task);
}

// Driver interrupt callbacks

void pbdrv_bluetooth_stm32_cc2640_srdy_irq(bool srdy) {
    spi_srdy = srdy;
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

    (void)size;

    switch (event) {
        case HCI_COMMAND_COMPLETE_EVENT:
            hci_command_complete = true;
            break;

        case HCI_LE_EXT_EVENT: {
            uint16_t event_code = (data[1] << 8) | data[0];
            HCI_StatusCodes_t status = data[2];
            uint16_t connection_handle = (data[4] << 8) | data[3];
            uint8_t pdu_len = data[5];

            (void)status;
            (void)pdu_len;

            switch (event_code) {
                case ATT_EVENT_EXCHANGE_MTU_REQ: {
                    attExchangeMTURsp_t rsp;

                    rsp.serverRxMTU = 158;
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
                                    GATT_PROP_WRITE_NO_RSP | GATT_PROP_WRITE | GATT_PROP_NOTIFY,
                                    pbio_pybricks_control_char_uuid);
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
                        default:
                            DBG("unhandled read by type req: %04X", type);
                            break;
                    }
                }
                break;

                case ATT_EVENT_READ_BY_TYPE_RSP: {
                    // Assuming that LEGO Powered Up remote is the only thing
                    // that provides this event type
                    remote_lwp3_char_handle = (data[8] << 8) | data[7];
                    // REVISIT: could also be bleTimeout
                }
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
                    } else if (handle == pybricks_char_handle + 1) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE - 1];

                        buf[0] = pybricks_notify_en;
                        buf[1] = 0;
                        rsp.len = 2;
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

                    DBG("w: %04X %04X %d", char_handle, uart_tx_char_handle, pdu_len - 4);
                    if (char_handle == pybricks_char_handle) {
                        if (receive_handler) {
                            receive_handler(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS, &data[10], pdu_len - 4);
                        }
                    } else if (char_handle == pybricks_char_handle + 1) {
                        pybricks_notify_en = data[10];
                        DBG("noti: %d", pybricks_notify_en);
                    } else if (char_handle == uart_rx_char_handle) {
                        if (receive_handler) {
                            receive_handler(PBDRV_BLUETOOTH_CONNECTION_UART, &data[10], pdu_len - 4);
                        }
                    } else if (char_handle == uart_tx_char_handle + 1) {
                        uart_tx_notify_en = data[10];
                        DBG("noti: %d", uart_tx_notify_en);
                    } else {
                        DBG("unhandled write req: %04X", char_handle);
                    }
                    if (!command) {
                        ATT_WriteRsp(connection_handle);
                    }
                }
                break;

                case ATT_EVENT_HANDLE_VALUE_NOTI: {
                    // TODO: match callback to handle
                    // uint8_t attr_handle = (data[7] << 8) | data[6];
                    if (notification_handler) {
                        notification_handler(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_HANDSET, &data[8], pdu_len - 2);
                    }
                }
                break;

                case GAP_DEVICE_DISCOVERY_DONE:
                    // TODO: do something with this - occurs when scanning is complete
                    break;

                case GAP_LINK_ESTABLISHED:
                    if (data[12] == GAP_PROFILE_PERIPHERAL) {
                        // we currently only allow connection from one central
                        conn_handle = (data[11] << 8) | data[10];
                        DBG("link: %04x", conn_handle);
                    } else if (data[12] == GAP_PROFILE_CENTRAL) {
                        // we currently only allow connection to one LEGO Powered Up remote peripheral
                        remote_handle = (data[11] << 8) | data[10];
                    }
                    break;

                case GAP_LINK_TERMINATED: {
                    DBG("bye: %04x", connection_handle);
                    if (conn_handle == connection_handle) {
                        conn_handle = NO_CONNECTION;
                        pybricks_notify_en = false;
                        uart_tx_notify_en = false;
                    } else if (remote_handle == connection_handle) {
                        remote_handle = NO_CONNECTION;
                        remote_lwp3_char_handle = NO_CONNECTION;
                    }
                }
                break;

                case GAP_LINK_PARAM_UPDATE:
                    // we get this event, but don't need to do anything about it
                    break;

                case GAP_DEVICE_INFORMATION:
                    advertising_data_received = true;
                    break;

                case HCI_EXT_SET_TX_POWER_EVENT:
                case HCI_EXT_SET_LOCAL_SUPPORTED_FEATURES_EVENT:
                case HCI_EXT_SET_BDADDR_EVENT:
                case GAP_DEVICE_INIT_DONE:
                case GAP_ADVERT_DATA_UPDATE_DONE:
                case GAP_MAKE_DISCOVERABLE_DONE:
                case GAP_END_DISCOVERABLE_DONE:
                    hci_command_complete = true;
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

    *rx_size = read_buf[2];

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
    uint8_t buf[1];

    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    buf[0] = GAPBOND_PAIRING_MODE_NO_PAIRING;
    GAP_BondMgrSetParameter(GAPBOND_PAIRING_MODE, 1, buf);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

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
    buf[0] = 0; // disabled
    GAP_BondMgrSetParameter(GAPBOND_BONDING_ENABLED, 1, buf);
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

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_GEN_DISC_SCAN_INT, 450);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_GEN_DISC_SCAN_WIND, 450);
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

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_SCAN_INT, 450);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_SCAN_WIND, 450);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, 60);
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

    PT_END(pt);
}

// Initializes the Bluetooth chip
// this function is largely inspired by the LEGO bootloader
static PT_THREAD(hci_init(struct pt *pt)) {
    static struct pt child_pt;

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

    // init GATT layer

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_deviceInit(GAP_PROFILE_PERIPHERAL | GAP_PROFILE_CENTRAL, 8, NULL, NULL, 0);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // set Tx power level

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_EXT_setTxPower(HCI_EXT_CC26XX_TX_POWER_0_DBM);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // get local version info
    // TODO: skip this - it is for info only

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_readLocalVersionInfo();
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    HCI_EXT_setLocalSupportedFeatures(HCI_EXT_LOCAL_FEATURE_ENCRYTION);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // get LEGO hardware version
    // TODO: skip this - it is for info only

    PT_WAIT_WHILE(pt, write_xfer_size);
    Util_readLegoHwVersion();
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    // get LEGO firmware version
    // TODO: skip this - it is for info only

    PT_WAIT_WHILE(pt, write_xfer_size);
    Util_readLegoFwVersion();
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_SPAWN(pt, &child_pt, gatt_init(&child_pt));
    PT_SPAWN(pt, &child_pt, gap_init(&child_pt));

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
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 4, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(pbio_pybricks_control_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CLIENT_CHAR_CFG_UUID, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);

    // the response to the last GATT_AddAttribute contains the first and last handles
    // that were allocated.
    pybricks_service_handle = (read_buf[13] << 8) | read_buf[12];
    pybricks_service_end_handle = (read_buf[15] << 8) | read_buf[14];
    pybricks_char_handle = pybricks_service_handle + 2;
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
        bluetooth_ready = pybricks_notify_en = uart_tx_notify_en = false;
        conn_handle = remote_handle = remote_lwp3_char_handle = NO_CONNECTION;
        PROCESS_EXIT();
    });

    PROCESS_BEGIN();

start:
    // take Bluetooth chip out of reset
    bluetooth_reset(RESET_STATE_OUT_HIGH);

    // not sure why we need this
    etimer_set(&timer, clock_from_msec(150));
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
    bluetooth_reset(RESET_STATE_INPUT);

    // bluetooth chip should not have any messages for us yet, so spi_srdy
    // should be false, if not wait a while and try resetting the chip
    if (spi_srdy) {
        static bool timed_out = false;

        etimer_set(&timer, clock_from_msec(500));

        while (spi_srdy) {
            PROCESS_WAIT_EVENT();
            if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
                timed_out = true;
            }
        }

        if (timed_out) {
            etimer_set(&timer, clock_from_msec(3000));
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
            bluetooth_reset(RESET_STATE_OUT_LOW);
            etimer_set(&timer, clock_from_msec(150));
            PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
            goto start;
        }
    }

    etimer_set(&timer, clock_from_msec(100));
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

    static pbio_task_t task;
    pbio_task_init(&task, init_task, NULL);
    pbio_task_start(task_queue, &task);

    while (true) {
        static uint8_t real_write_xfer_size;

        PROCESS_WAIT_UNTIL(spi_srdy || write_xfer_size);

        spi_set_mrdy(true);

        // Need to make a copy of the write transfer size for the case where we
        // are reading only. In this case we still have to set write_xfer_size
        // to a non-zero value to indicate that write_buf is being used. But we
        // still need the real size later to figure out the actual size of the
        // SPI transfer.
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
        if (get_npi_rx_size(&read_xfer_size) && read_xfer_size > real_write_xfer_size - 4) {
            xfer_size = read_xfer_size + 4;
        } else {
            xfer_size = real_write_xfer_size;
        }

        // read the remaining message
        spi_xfer_complete = false;
        pdata->spi_start_xfer(&write_buf[NPI_SPI_HEADER_LEN],
            &read_buf[NPI_SPI_HEADER_LEN], xfer_size);
        PROCESS_WAIT_UNTIL(spi_xfer_complete);

        // HACK: SRDY can transition from low and back to high in the time
        // between we set MRDY and when we read SRDY again. So we use a timer
        // prevent a lockup in case we miss detecting the transitions.

        // See Δt6 + Δt7 in the timing diagram at:
        // https://dev.ti.com/tirex/explore/content/simplelink_cc13x2_26x2_sdk_3_10_00_53/docs/ble5stack/ble_user_guide/html/ble-stack-common/npi-index.html#npi-handshake

        // This document suggests that this timing varies from 0.181ms to 1.2 ms.
        // http://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/538/3583.BLE-SPI-Driver-Design-External.pdf

        // REVISIT: maybe there is a way to get individual oneshots for the
        // rising and falling edges of the interrupt instead of the timer hack?

        etimer_set(&timer, clock_from_msec(2));
        spi_set_mrdy(false);
        PROCESS_WAIT_UNTIL(!spi_srdy || (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)));

        // set to 0 to indicate that xfer is complete
        write_xfer_size = 0;

        if (read_xfer_size) {
            // handle the received data
            if (read_buf[NPI_SPI_HEADER_LEN] == HCI_EVENT_PACKET) {
                handle_event(&read_buf[NPI_SPI_HEADER_LEN + 1]);
            }
            // TODO: do we need to handle ACL packets (HCI_ACLDATA_PKT)?
        }

        pbio_task_queue_run_once(task_queue);
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

    process_poll(&pbdrv_bluetooth_spi_process);

    return bleSUCCESS;
}

#endif // PBDRV_CONFIG_BLUETOOTH_STM32_CC2640
