// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// Bluetooth for STM32 MCU with STMicro BlueNRG-MS

// This file hard-codes hardware access to save code size instead of using
// platform.c since this is only used on the BOOST Move hub.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_STM32_BLUENRG

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bluetooth.h"

#include <pbdrv/bluetooth.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>
#include <pbio/protocol.h>
#include <pbio/util.h>
#include <pbio/version.h>
#include <pbsys/config.h>
#include <pbsys/storage.h>


#include <lego/lwp3.h>
#include <stm32f070xb.h>

#include <bluenrg_aci.h>
#include <bluenrg_gap.h>
#include <hci_le.h>
#include <hci_tl.h>

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

// hub name goes in special section so that it can be modified when flashing firmware
__attribute__((section(".name")))
char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

static char pbdrv_bluetooth_fw_version[5]; // 0.0a

// used to identify which hub - Device Information Service (DIS).
// 0x2A50 - service UUID - PnP ID characteristic UUID
// 0x01 - Vendor ID Source Field - Bluetooth SIG-assigned ID
// 0x0397 - Vendor ID Field - LEGO company identifier
// 0x0040 - Product ID Field - Move hub device ID
// 0x0000 - Product Version Field - not applicable to Move hub
#define PNP_ID "\x50\x2a\x01\x97\x03\x40\x00\x00\x00"

// bluetooth address is set at factory at this address
#define FLASH_BD_ADDR ((const uint8_t *)0x08004ffa)

// size of the BlueNRG SPI header
#define BLUENRG_HEADER_SIZE 5
// value returned in READY byte of BlueNRG SPI header when interface is ready
#define BLUENRG_READY 2

// max data size for Nordic UART characteristics
#define NUS_CHAR_SIZE (ATT_MTU - 3)

// BlueNRG header data for SPI write xfer
static const uint8_t write_header_tx[BLUENRG_HEADER_SIZE] = { 0x0a };
// BlueNRG header data for SPI read xfer
static const uint8_t read_header_tx[BLUENRG_HEADER_SIZE] = { 0x0b };
// Rx buffer for BlueNRG header
static uint8_t header_rx[BLUENRG_HEADER_SIZE];

// Tx buffer for SPI writes
static uint8_t write_buf[HCI_MAX_PAYLOAD_SIZE];
// Rx buffer for SPI reads
static uint8_t read_buf[HCI_MAX_PAYLOAD_SIZE];
// Dummy Tx data for read xfers
static const uint8_t dummy_write_buf[1] = { 0xff };
// Dummy Rx data for write xfers
static uint8_t dummy_read_buf[1];

// size of current SPI xfer Tx data (not including BlueNRG read/write header)
// value is set to 0 when Tx is complete
static uint8_t write_xfer_size;

// reflects state of SPI_IRQ pin
volatile bool spi_irq;
// set to false when xfer is started and true when xfer is complete
volatile bool spi_xfer_complete;

// set to false when hci command is started and true when command is completed
static bool hci_command_complete;
// set to false when hci command is started and true when command status is received
static bool hci_command_status;
// used to synchronize advertising data handler
static bool advertising_data_received;
// handle to connected Bluetooth device
static uint16_t conn_handle;

// used to wait for Evt_Blue_Gatt_Tx_Pool_Available
static bool tx_pool_available;

// Pybricks GATT service handles
static uint16_t pybricks_service_handle;
static uint16_t pybricks_command_event_char_handle;
static uint16_t pybricks_hub_capabilities_char_handle;

// Nordic UART GATT service handles
static uint16_t uart_service_handle, uart_rx_char_handle, uart_tx_char_handle;

static bool pybricks_notify_en;
static bool uart_tx_notify_en;

static const pbdrv_gpio_t reset_gpio = { .bank = GPIOB, .pin = 6 };
static const pbdrv_gpio_t cs_gpio = { .bank = GPIOB, .pin = 12 };
static const pbdrv_gpio_t irq_gpio = { .bank = GPIOD, .pin = 2 };
static const pbdrv_gpio_t mosi_gpio = { .bank = GPIOC, .pin = 3 };
static const pbdrv_gpio_t miso_gpio = { .bank = GPIOC, .pin = 2 };
static const pbdrv_gpio_t sck_gpio = { .bank = GPIOB, .pin = 13 };

static pbdrv_bluetooth_peripheral_t peripheral_singleton;

pbdrv_bluetooth_peripheral_t *pbdrv_bluetooth_peripheral_get_by_index(uint8_t index) {
    // This platform supports only a single peripheral instance. Some of its
    // states are global variables listed above. This single instance is used
    // troughout the event handler.
    return &peripheral_singleton;
}

bool pbdrv_bluetooth_peripheral_is_connected(pbdrv_bluetooth_peripheral_t *peri) {
    return peri == &peripheral_singleton && peri->con_handle != 0;
}

/**
 * Converts a BlueNRG-MS error code to a PBIO error code.
 * @param [in]  status  The BlueNRG-MS error code.
 * @return              The PBIO error code.
 */
static pbio_error_t ble_error_to_pbio_error(tBleStatus status) {
    if (status == BLE_STATUS_SUCCESS) {
        return PBIO_SUCCESS;
    }
    if (status == BLE_STATUS_FAILED) {
        return PBIO_ERROR_INVALID_OP;
    }
    if (status == BLE_STATUS_TIMEOUT) {
        return PBIO_ERROR_TIMEDOUT;
    }
    return PBIO_ERROR_FAILED;
}

/**
 * Gets a vendor-specific event for a specific connection.
 * @param [out] event       The vendor-specific event.
 * @return                  The event payload or NULL if there is no pending
 *                          vendor-specific event.
 */
static void *get_vendor_event(uint16_t *event) {
    hci_uart_pckt *packet = (void *)read_buf;

    if (packet->type != HCI_EVENT_PKT) {
        return NULL;
    }

    hci_event_pckt *event_packet = (void *)packet->data;

    if (event_packet->evt != EVT_VENDOR) {
        return NULL;
    }

    *event = pbio_get_uint16_le(event_packet->data);

    return &event_packet->data[2];
}

/**
 * Sets the nRESET line on the Bluetooth chip.
 */
static void bluetooth_reset(bool reset) {
    if (reset) {
        pbdrv_gpio_out_low(&reset_gpio);
    } else {
        pbdrv_gpio_out_high(&reset_gpio);
    }
}

/**
 * Initializes the SPI connection to the Bluetooth chip.
 */
static void spi_init(void) {
    // SPI2 pin mux

    // SPI_CS
    pbdrv_gpio_out_high(&cs_gpio);
    // SPI_IRQ
    pbdrv_gpio_input(&irq_gpio);
    pbdrv_gpio_set_pull(&irq_gpio, PBDRV_GPIO_PULL_DOWN);
    // SPI_MOSI
    pbdrv_gpio_alt(&mosi_gpio, 1);
    // SPI_MISO
    pbdrv_gpio_alt(&miso_gpio, 1);
    // SPI_SCK
    pbdrv_gpio_alt(&sck_gpio, 0);

    // DMA

    DMA1_Channel4->CPAR = DMA1_Channel5->CPAR = (uint32_t)&SPI2->DR;

    NVIC_SetPriority(DMA1_Channel4_5_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

    // SPI2

    // set as master and clock /8
    #define SPI_CR1_BR_DIV8 SPI_CR1_BR_1
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV8 | SPI_CR1_SSM;
    // enable DMA rx/tx, chip select (even though it is unused?), rx not empty irq, 8-bit word size, trigger rx irq on 8-bit
    #define SPI_CR2_DS_8BIT (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2)
    SPI2->CR2 = SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN | SPI_CR2_SSOE | SPI_CR2_RXNEIE | SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;

    // LEGO Firmware doesn't do this, but we actually use IRQ for SPI_IRQ pin
    // this is needed since we use __WFI() sometimes
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PD;
    EXTI->IMR |= EXTI_EMR_MR2;
    EXTI->RTSR |= EXTI_RTSR_RT2;
    EXTI->FTSR |= EXTI_FTSR_FT2;
    NVIC_SetPriority(EXTI2_3_IRQn, 3);
    NVIC_EnableIRQ(EXTI2_3_IRQn);
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
    // c5f50001-8280-46da-89f4-6d8051e4aeef
    static const uint8_t service_uuids[] = {
        AD_TYPE_128_BIT_SERV_UUID,
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
        0xda, 0x46, 0x80, 0x82, 0x01, 0x00, 0xf5, 0xc5 // Pybricks service UUID
    };

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    // TODO: LEGO firmware also includes Conn_Interval_Min, Conn_Interval_Max.
    // Do we need these?
    uint8_t response_data[25];
    response_data[0] = sizeof(PNP_ID);
    response_data[1] = AD_TYPE_SERVICE_DATA;
    memcpy(&response_data[2], PNP_ID, sizeof(PNP_ID) - 1);
    uint8_t hub_name_len = strlen(pbdrv_bluetooth_hub_name);
    response_data[11] = hub_name_len + 1;
    response_data[12] = AD_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&response_data[13], pbdrv_bluetooth_hub_name, hub_name_len);
    _Static_assert(13 + sizeof(pbdrv_bluetooth_hub_name) - 1 <= 31, "scan response is 31 octet max");

    hci_le_set_scan_response_data_begin(13 + hub_name_len, response_data);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // hci_le_set_scan_response_data_end();

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_set_discoverable_begin(ADV_IND, 0, 0, STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
        0, NULL, sizeof(service_uuids), service_uuids, 0, 0);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_gap_set_discoverable_end();

    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_ADVERTISING_PYBRICKS;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    // REVISIT: might need to delete advertising data here

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_set_non_discoverable_begin();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_gap_set_non_discoverable_end();

    // This protothread is also shared with stop broadcasting. Either way,
    // nothing is advertising or broadcasting after this, so reset that state.
    // even if it wasn't active.
    pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}


bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {

    if (connection == PBDRV_BLUETOOTH_CONNECTION_HCI) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_LE && conn_handle) {
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

    PBIO_OS_ASYNC_BEGIN(state);

retry:
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_update_char_value_begin(pybricks_service_handle, pybricks_command_event_char_handle, 0, size, data);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    tBleStatus ret = aci_gatt_update_char_value_end();

    if (ret == BLE_STATUS_INSUFFICIENT_RESOURCES) {
        // this will happen if notifications are enabled and the previous
        // changes haven't been sent over the air yet
        tx_pool_available = false;
        PBIO_OS_AWAIT_UNTIL(state, tx_pool_available);
        goto retry;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context) {
    pbdrv_bluetooth_peripheral_t *peri = context;

    // Scan and connect timeout, if applicable.
    bool timed_out = peri->config->timeout && pbio_os_timer_is_expired(&peri->timer);

    // Operation can be explicitly cancelled or automatically on inactivity.
    if (!peri->cancel) {
        peri->cancel = pbio_os_timer_is_expired(&peri->watchdog);
    }

    PBIO_OS_ASYNC_BEGIN(state);

    // start scanning
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_start_general_conn_establish_proc_begin(ACTIVE_SCAN, 0x0030, 0x0030, STATIC_RANDOM_ADDR, 0);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    peri->status = aci_gap_start_general_conn_establish_proc_end();

try_again:

    for (;;) {
        advertising_data_received = false;
        PBIO_OS_AWAIT_UNTIL(state, advertising_data_received || peri->cancel || timed_out);

        if (!advertising_data_received) {
            // Things didn't work out, so stop scanning.
            PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
            aci_gap_terminate_gap_procedure_begin(GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC);
            PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
            // aci_gap_terminate_gap_procedure_end();
            return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
        }

        le_advertising_info *subevt = (void *)&read_buf[5];

        // Context specific advertisement filter.
        pbdrv_bluetooth_ad_match_result_flags_t adv_flags = peri->config->match_adv(peri->user, subevt->evt_type, subevt->data_RSSI, NULL, subevt->bdaddr, peri->bdaddr);

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

        // save the Bluetooth address for later
        peri->bdaddr_type = subevt->bdaddr_type;
        memcpy(peri->bdaddr, subevt->bdaddr, 6);

        break;
    }

    for (;;) {
        advertising_data_received = false;
        PBIO_OS_AWAIT_UNTIL(state, advertising_data_received || peri->cancel || timed_out);

        if (!advertising_data_received) {
            // Things didn't work out, so stop scanning.
            PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
            aci_gap_terminate_gap_procedure_begin(GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC);
            PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
            // aci_gap_terminate_gap_procedure_end();
            return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
        }

        // If the response data is not right or if the address doesn't match advertisement, keep scanning.
        le_advertising_info *subevt = (void *)&read_buf[5];
        const char *detected_name = (char *)&subevt->data_RSSI[2];
        pbdrv_bluetooth_ad_match_result_flags_t rsp_flags = peri->config->match_adv_rsp(peri->user, subevt->evt_type, NULL, detected_name, subevt->bdaddr, peri->bdaddr);
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
    aci_gap_terminate_gap_procedure_begin(GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    peri->status = aci_gap_terminate_gap_procedure_end();

    // REVISIT: might need to wait for procedure complete event here

    // connect
    assert(!peri->con_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_create_connection_begin(0x0060, 0x0030, peri->bdaddr_type, peri->bdaddr,
        STATIC_RANDOM_ADDR, 0x0010 >> 1, 0x0030 >> 1, 4, 720 / 10, 0x0010, 0x0030);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    peri->status = aci_gap_create_connection_end();

    // Await connection to be ready unless cancelled sooner.
    PBIO_OS_AWAIT_UNTIL(state, peri->cancel || timed_out || peri->con_handle);

    if (peri->cancel || timed_out) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        aci_gap_terminate_gap_procedure_begin(GAP_DIRECT_CONNECTION_ESTABLISHMENT_PROC);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
        // aci_gap_terminate_gap_procedure_end();
        return peri->cancel ? PBIO_ERROR_CANCELED : PBIO_ERROR_TIMEDOUT;
    }

    PBIO_OS_ASYNC_END(ble_error_to_pbio_error(peri->status));
}

pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context) {
    pbdrv_bluetooth_peripheral_t *peri = context;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);

    uint8_t uuid_le[16];
    uint8_t uuid_type;
    if (peri->char_now->uuid16) {
        pbio_set_uint16_le(uuid_le, peri->char_now->uuid16);
        uuid_type = UUID_TYPE_16;
    } else {
        pbio_uuid128_reverse_copy(uuid_le, peri->char_now->uuid128);
        uuid_type = UUID_TYPE_128;
    }
    uint16_t handle_max = /* peri->char_now->handle_max ? peri->char_now->handle_max : */ 0xffff; // Not implemented due to build size limitations.
    aci_gatt_disc_charac_by_uuid_begin(peri->con_handle, 0x0001, handle_max, uuid_type, uuid_le);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    peri->status = aci_gatt_disc_charac_by_uuid_end();

    PBIO_OS_AWAIT_UNTIL(state, ({
        void *payload;
        uint16_t event;
        (payload = get_vendor_event(&event))
        && ({
            if (event == EVT_BLUE_GATT_DISC_READ_CHAR_BY_UUID_RESP) {
                evt_gatt_disc_read_char_by_uuid_resp *subevt = payload;
                // Filtering by properties is not implemented due to build size
                // limitations. This means it will find only one characteristic
                // even if there are multiple matches. There is no guarantee
                // which one it will find, but probably the last one.
                if (subevt->conn_handle == peri->con_handle) {
                    peri->char_now->handle = subevt->attr_handle;
                }
            }
            event == EVT_BLUE_GATT_PROCEDURE_COMPLETE;
        }) && ({
            evt_gatt_procedure_complete *subevt = payload;
            subevt->conn_handle == peri->con_handle;
        });
    }));

    // If notifications are not requested, we're done.
    if (!peri->char_now->request_notification) {
        return ble_error_to_pbio_error(peri->status);
    }

    static const uint16_t enable = 0x0001;

retry:
    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_write_charac_value_begin(peri->con_handle, peri->char_now->handle + 2, sizeof(enable), (const uint8_t *)&enable);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    peri->status = aci_gatt_write_charac_value_end();

    if (peri->status != BLE_STATUS_SUCCESS) {

        if (peri->status == BLE_STATUS_NOT_ALLOWED) {
            goto retry;
        }

        return ble_error_to_pbio_error(peri->status);
    }

    evt_gatt_procedure_complete *payload;
    PBIO_OS_AWAIT_UNTIL(state, ({
        uint16_t event;
        (payload = get_vendor_event(&event))
        && event == EVT_BLUE_GATT_PROCEDURE_COMPLETE
        && payload->conn_handle == peri->con_handle;
    }));

    peri->status = payload->error_code;

    PBIO_OS_ASYNC_END(ble_error_to_pbio_error(peri->status));
}


pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context) {
    pbdrv_bluetooth_peripheral_t *peri = context;

    evt_gatt_procedure_complete *payload;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);

    aci_gatt_write_charac_value_begin(peri->con_handle, peri->char_handle + 1, peri->char_size, peri->char_data);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    tBleStatus status = aci_gatt_write_charac_value_end();

    if (status != BLE_STATUS_SUCCESS) {
        return ble_error_to_pbio_error(status);
    }

    PBIO_OS_AWAIT_UNTIL(state, ({
        if (peri->con_handle == 0) {
            return PBIO_ERROR_NO_DEV;
        }

        uint16_t event;
        (payload = get_vendor_event(&event))
        && event == EVT_BLUE_GATT_PROCEDURE_COMPLETE
        && payload->conn_handle == peri->con_handle;
    }));

    PBIO_OS_ASYNC_END(ble_error_to_pbio_error(payload->error_code));
}

pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = context;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_terminate_begin(peri->con_handle, HCI_OE_USER_ENDED_CONNECTION);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    aci_gap_terminate_end();

    PBIO_OS_AWAIT_UNTIL(state, peri->con_handle == 0);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}


pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context) {

    tBleStatus status;

    PBIO_OS_ASYNC_BEGIN(state);

    if (pbdrv_bluetooth_advertising_state != PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        aci_gap_set_non_connectable_begin(ADV_NONCONN_IND, STATIC_RANDOM_ADDR);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
        status = aci_gap_set_non_connectable_end();

        if (status != BLE_STATUS_SUCCESS) {
            pbio_error_t err = ble_error_to_pbio_error(status);
            // Broadcasting does not work while connected to the computer. But
            // returning an error means that Move Hub programs with
            // broadcasting can never run while connected, which makes it very
            // impractical to test any program. So mark as success.
            if (err == PBIO_ERROR_INVALID_OP) {
                return PBIO_SUCCESS;
            }
            return err;
        }

        // These AD types are left over from connectable discovery and need
        // to be deleted _after_ starting non-connectable advertising.

        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        aci_gap_delete_ad_type_begin(AD_TYPE_128_BIT_SERV_UUID);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);

        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        aci_gap_delete_ad_type_begin(AD_TYPE_TX_POWER_LEVEL);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);

        // Errors from deleting are ignored since we should only get an error
        // if the AD does not exist, which is OK.

        pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING;
    }

    // This has to be done _after_ other data is delete to make sure it fits.

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_update_adv_data_begin(pbdrv_bluetooth_broadcast_data_size, pbdrv_bluetooth_broadcast_data);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    status = aci_gap_update_adv_data_end();

    PBIO_OS_ASYNC_END(ble_error_to_pbio_error(status));
}

pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context) {

    tBleStatus status;

    PBIO_OS_ASYNC_BEGIN(state);

    // NB: we should probably be using aci_gap_start_observation_procedure_begin
    // here, but we already use aci_gap_start_general_conn_establish_proc_begin
    // elsewhere, so this reduces code size and we would also have to enable
    // the observer role which would use more RAM in the Bluetooth chip

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_start_general_conn_establish_proc_begin(PASSIVE_SCAN, 0x30, 0x30, STATIC_RANDOM_ADDR, 0);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
    status = aci_gap_start_general_conn_establish_proc_end();
    if (status == BLE_STATUS_SUCCESS) {
        pbdrv_bluetooth_is_observing = true;
    }
    PBIO_OS_ASYNC_END(ble_error_to_pbio_error(status));
}


pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    if (!pbdrv_bluetooth_is_observing) {
        return PBIO_SUCCESS;
    }

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_terminate_gap_procedure_begin(GAP_GENERAL_CONNECTION_ESTABLISHMENT_PROC);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // tBleStatus status = aci_gap_terminate_gap_procedure_end();

    // if (status != BLE_STATUS_SUCCESS) {
    //     task->status = ble_error_to_pbio_error(status);
    //     PT_EXIT(pt);
    // }

    // TODO: wait for Evt_Blue_Gap_Procedure_Completed

    pbdrv_bluetooth_is_observing = false;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// overrides weak function in start_*.S
void DMA1_Channel4_5_IRQHandler(void) {
    // if CH4 transfer complete
    if (DMA1->ISR & DMA_ISR_TCIF4) {
        // clear interrupt
        DMA1->IFCR = DMA_IFCR_CTCIF4;
        // disable CH4
        DMA1_Channel4->CCR &= ~DMA_CCR_EN;

        // notify that SPI xfer is complete
        spi_xfer_complete = true;
        pbio_os_request_poll();
    }
}

// overrides weak function in startup_*.S
void EXTI2_3_IRQHandler(void) {
    spi_irq = !!(GPIOD->IDR & GPIO_IDR_2);

    // clear the interrupt
    EXTI->PR = EXTI_PR_PR2;

    pbio_os_request_poll();
}

static inline void spi_enable_cs(void) {
    pbdrv_gpio_out_low(&cs_gpio);
}

static inline void spi_disable_cs(void) {
    pbdrv_gpio_out_high(&cs_gpio);
}

// configures and starts an SPI xfer
static void spi_start_xfer(const uint8_t *tx_buf, uint8_t *rx_buf, uint8_t xfer_size) {
    // reset the xfer complete flag
    spi_xfer_complete = false;

    // hopefully this shouldn't actually block, but we can't disable SPI while
    // it is busy, so just in case...
    while (SPI2->SR & SPI_SR_BSY) {
    }

    // disable the SPI so we can configure it
    SPI2->CR1 &= ~SPI_CR1_SPE;

    // configure DMA
    DMA1_Channel5->CCR = 0;
    DMA1_Channel4->CCR = 0;
    DMA1_Channel5->CMAR = (uint32_t)tx_buf;
    DMA1_Channel5->CNDTR = xfer_size;
    DMA1_Channel4->CMAR = (uint32_t)rx_buf;
    DMA1_Channel4->CNDTR = xfer_size;
    // dummy bufs are only one byte, so don't enable memory increment for them
    DMA1_Channel4->CCR = (rx_buf == dummy_read_buf ? 0 : DMA_CCR_MINC) | DMA_CCR_TCIE | DMA_CCR_EN;
    DMA1_Channel5->CCR = (tx_buf == dummy_write_buf ? 0 : DMA_CCR_MINC) | DMA_CCR_DIR | DMA_CCR_EN;

    // enable SPI to start the xfer
    SPI2->CR1 |= SPI_CR1_SPE;
}

// gets the available tx and rx buffer sizes returned in the BlueNRG SPI header
// returns true if the header is valid, otherwise false
static bool get_bluenrg_buf_size(uint8_t *wbuf, uint8_t *rbuf) {
    if (header_rx[0] != BLUENRG_READY) {
        return false;
    }

    *wbuf = header_rx[1];
    *rbuf = header_rx[3];

    return true;
}

// assigned to one of RESET_* from bluenrg_hal_aci.h
static uint8_t reset_reason;

static pbio_error_t init_device_information_service(pbio_os_state_t *state, void *context) {
    static const uint8_t device_information_service_uuid[] = { 0x0A, 0x18 }; // 0x180A
    static const uint8_t firmware_version_char_uuid[] = { 0x26, 0x2A }; // 0x2A26
    static const uint8_t software_version_char_uuid[] = { 0x28, 0x2A }; // 0x2A28

    static uint16_t service_handle, fw_ver_char_handle, sw_ver_char_handle, pnp_id_char_handle;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_serv_begin(UUID_TYPE_16, device_information_service_uuid, PRIMARY_SERVICE, 7);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_serv_end(&service_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_char_begin(service_handle, UUID_TYPE_16, firmware_version_char_uuid,
        sizeof(PBIO_VERSION_STR) - 1, CHAR_PROP_READ, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_CONSTANT);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_char_end(&fw_ver_char_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_update_char_value_begin(service_handle, fw_ver_char_handle,
        0, sizeof(PBIO_VERSION_STR) - 1, PBIO_VERSION_STR);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_update_char_value_end();

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_char_begin(service_handle, UUID_TYPE_16, software_version_char_uuid,
        sizeof(PBIO_PROTOCOL_VERSION_STR) - 1, CHAR_PROP_READ, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_CONSTANT);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_char_end(&sw_ver_char_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_update_char_value_begin(service_handle, sw_ver_char_handle,
        0, sizeof(PBIO_PROTOCOL_VERSION_STR) - 1, PBIO_PROTOCOL_VERSION_STR);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_update_char_value_end();

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_char_begin(service_handle, UUID_TYPE_16, (const uint8_t *)PNP_ID,
        sizeof(PNP_ID) - 3, CHAR_PROP_READ, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_CONSTANT);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_char_end(&pnp_id_char_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_update_char_value_begin(service_handle, pnp_id_char_handle,
        0, sizeof(PNP_ID) - 3, &PNP_ID[2]);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_gatt_update_char_value_end();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_error_t init_pybricks_service(pbio_os_state_t *state, void *context) {
    // c5f50001-8280-46da-89f4-6d8051e4aeef
    static const uint8_t pybricks_service_uuid[] = {
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
        0xda, 0x46, 0x80, 0x82, 0x01, 0x00, 0xf5, 0xc5
    };

    // c5f50002-8280-46da-89f4-6d8051e4aeef
    static const uint8_t pybricks_command_event_char_uuid[] = {
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
        0xda, 0x46, 0x80, 0x82, 0x02, 0x00, 0xf5, 0xc5
    };

    // c5f50003-8280-46da-89f4-6d8051e4aeef
    static const uint8_t pybricks_hub_capabilities_char_uuid[] = {
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
        0xda, 0x46, 0x80, 0x82, 0x03, 0x00, 0xf5, 0xc5
    };

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_serv_begin(UUID_TYPE_128, pybricks_service_uuid, PRIMARY_SERVICE, 6);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_serv_end(&pybricks_service_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_char_begin(pybricks_service_handle, UUID_TYPE_128, pybricks_command_event_char_uuid,
        ATT_MTU - 3, CHAR_PROP_WRITE | CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE,
        GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_char_end(&pybricks_command_event_char_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_char_begin(pybricks_service_handle, UUID_TYPE_128, pybricks_hub_capabilities_char_uuid,
        PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE, CHAR_PROP_READ, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_CONSTANT);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_char_end(&pybricks_hub_capabilities_char_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    {
        uint8_t buf[PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE];
        pbio_pybricks_hub_capabilities(buf, ATT_MTU - 3, PBSYS_CONFIG_APP_FEATURE_FLAGS, pbsys_storage_get_maximum_program_size(), 0);
        aci_gatt_update_char_value_begin(pybricks_service_handle, pybricks_hub_capabilities_char_handle,
            0, PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE, buf);
    }
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_gatt_update_char_value_end();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// processes an event received from the Bluetooth chip
static void handle_event(hci_event_pckt *event) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    switch (event->evt) {
        case EVT_DISCONN_COMPLETE: {
            evt_disconn_complete *evt = (evt_disconn_complete *)event->data;
            if (evt->handle == conn_handle) {
                conn_handle = 0;
                pybricks_notify_en = false;
                uart_tx_notify_en = false;
            } else if (evt->handle == peri->con_handle) {
                peri->con_handle = 0;
            }
        }
        break;

        case EVT_CMD_COMPLETE:
            hci_command_complete = true;
            break;

        case EVT_CMD_STATUS:
            hci_command_status = true;
            break;

        case EVT_LE_META_EVENT: {
            evt_le_meta_event *evt = (evt_le_meta_event *)event->data;
            switch (evt->subevent) {
                case EVT_LE_CONN_COMPLETE: {
                    evt_le_connection_complete *subevt = (evt_le_connection_complete *)evt->data;
                    if (subevt->role == GAP_PERIPHERAL_ROLE) {
                        conn_handle = subevt->handle;
                        pbdrv_bluetooth_advertising_state = PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE;
                    } else {
                        peri->con_handle = subevt->handle;
                    }
                }
                break;

                case EVT_LE_ADVERTISING_REPORT: {
                    // NB: assumes num_reports is always 1
                    le_advertising_info *subevt = (void *)(evt->data + 1);

                    if (pbdrv_bluetooth_observe_callback) {
                        pbdrv_bluetooth_observe_callback(subevt->evt_type, subevt->data_RSSI,
                            subevt->data_length, subevt->data_RSSI[subevt->data_length]);
                    }

                    advertising_data_received = true;
                }
                break;
            }
        }
        break;

        case EVT_VENDOR: {
            evt_blue_aci *evt = (evt_blue_aci *)event->data;
            switch (evt->ecode) {
                case EVT_BLUE_HAL_INITIALIZED: {
                    evt_hal_initialized *subevt = (evt_hal_initialized *)evt->data;
                    reset_reason = subevt->reason_code;
                }
                break;

                case EVT_BLUE_GATT_ATTRIBUTE_MODIFIED: {
                    evt_gatt_attr_modified *subevt = (evt_gatt_attr_modified *)evt->data;
                    if (subevt->attr_handle == pybricks_command_event_char_handle + 2) {
                        pybricks_notify_en = subevt->att_data[0];
                    } else if (subevt->attr_handle == uart_rx_char_handle + 1) {
                        // not implemented
                    } else if (subevt->attr_handle == uart_tx_char_handle + 2) {
                        uart_tx_notify_en = subevt->att_data[0];
                    }
                }
                break;

                case EVT_BLUE_GATT_NOTIFICATION: {
                    evt_gatt_attr_notification *subevt = (void *)evt->data;
                    if (peri->config->notification_handler) {
                        peri->config->notification_handler(peri->user, subevt->attr_value, subevt->event_data_length - 2);
                    }
                }
                break;

                case EVT_BLUE_GATT_WRITE_PERMIT_REQ: {
                    evt_gatt_write_permit_req *subevt = (evt_gatt_write_permit_req *)evt->data;
                    pbio_pybricks_error_t err = PBIO_PYBRICKS_ERROR_INVALID_HANDLE;

                    if (subevt->attr_handle == pybricks_command_event_char_handle + 1) {
                        if (pbdrv_bluetooth_receive_handler) {
                            err = pbdrv_bluetooth_receive_handler(subevt->data, subevt->data_length);
                        }
                    }

                    aci_gatt_write_response_begin(subevt->conn_handle, subevt->attr_handle, !!err, err, subevt->data_length, subevt->data);
                }
                break;

                case EVT_BLUE_GATT_TX_POOL_AVAILABLE: {
                    // REVISIT: We might need to look at the event args for
                    // connection handle if we need to handle this in multiple
                    // places, e.g. for notifications and write without response.
                    tx_pool_available = true;
                }
                break;
            }
        }
        break;
    }

    pbio_os_request_poll();
}

// read message from BlueNRG chip
static pbio_error_t spi_read(pbio_os_state_t *state) {
    hci_uart_pckt *pckt = (hci_uart_pckt *)read_buf;
    uint8_t wbuf, rbuf;

    PBIO_OS_ASYNC_BEGIN(state);

retry:
    if (!spi_irq) {
        // if SPI_IRQ went away, reading will fail, so don't try
        return PBIO_ERROR_IO;
    }

    spi_enable_cs();

    // send the read header
    spi_start_xfer(read_header_tx, header_rx, BLUENRG_HEADER_SIZE);
    PBIO_OS_AWAIT_UNTIL(state, spi_xfer_complete);

    // keep retrying until the chip is ready to read
    if (!get_bluenrg_buf_size(&wbuf, &rbuf) || rbuf == 0) {
        // TODO: should probably have a timeout (and reset the chip after that?)
        spi_disable_cs();
        goto retry;
    }

    // read the message
    spi_start_xfer(dummy_write_buf, read_buf, rbuf);
    PBIO_OS_AWAIT_UNTIL(state, spi_xfer_complete);

    spi_disable_cs();

    // handle the received data (note: can't use switch statement here because
    // we are using protothreads)
    if (pckt->type == HCI_EVENT_PKT) {
        handle_event((hci_event_pckt *)pckt->data);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// write message to BlueNRG chip
static pbio_error_t spi_write(pbio_os_state_t *state) {
    uint8_t wbuf, rbuf;

    PBIO_OS_ASYNC_BEGIN(state);

retry:
    spi_enable_cs();

    // send the write header
    spi_start_xfer(write_header_tx, header_rx, BLUENRG_HEADER_SIZE);
    PBIO_OS_AWAIT_UNTIL(state, spi_xfer_complete);

    // keep retrying until the chip is ready to write
    if (!get_bluenrg_buf_size(&wbuf, &rbuf) || wbuf < write_xfer_size) {
        // TODO: should probably have a timeout (and reset the chip after that?)
        spi_disable_cs();
        goto retry;
    }

    // write the message
    spi_start_xfer(write_buf, dummy_read_buf, write_xfer_size);
    PBIO_OS_AWAIT_UNTIL(state, spi_xfer_complete);

    spi_disable_cs();

    // set to 0 to indicate that xfer is complete
    write_xfer_size = 0;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// implements function for BlueNRG library
void hci_send_req(struct hci_request *r) {
    hci_uart_pckt *pckt = (hci_uart_pckt *)write_buf;
    hci_command_hdr *hdr = (hci_command_hdr *)pckt->data;
    void *cmd = &write_buf[HCI_HDR_SIZE + HCI_COMMAND_HDR_SIZE];

    pckt->type = HCI_COMMAND_PKT;
    hdr->opcode = r->opcode;
    hdr->plen = r->clen;
    memcpy(cmd, r->cparam, r->clen);
    write_xfer_size = HCI_HDR_SIZE + HCI_COMMAND_HDR_SIZE + r->clen;

    hci_command_complete = hci_command_status = false;
}

// implements function for BlueNRG library
void hci_recv_resp(struct hci_response *r) {
    int offset = HCI_HDR_SIZE + HCI_EVENT_HDR_SIZE;

    assert(read_buf[0] == HCI_EVENT_PKT);

    // *_end() functions for command complete skip the evt_cmd_complete struct
    // when unpacking the event data
    if (read_buf[1] == EVT_CMD_COMPLETE) {
        offset += EVT_CMD_COMPLETE_SIZE;
    }

    memcpy(r->rparam, &read_buf[offset], r->rlen);
}

// Initializes the Bluetooth chip
// this function is largely inspired by the LEGO bootloader
static pbio_error_t hci_init(pbio_os_state_t *state, void *context) {
    static uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;

    PBIO_OS_ASYNC_BEGIN(state);

    // set the mode

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    {
        // NB: if we use mode 3, then there is not enough RAM for all of the
        // curent services/characteristics
        uint8_t mode = 4;
        aci_hal_write_config_data_begin(CONFIG_DATA_MODE_OFFSET, CONFIG_DATA_MODE_LEN, &mode);
    }
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_hal_write_config_data_end();

    // set the Bluetooth address

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    {
        // has to be in little-endian format, but stored in big-endian in flash
        uint8_t bd_addr[6];
        bd_addr[0] = FLASH_BD_ADDR[5];
        bd_addr[1] = FLASH_BD_ADDR[4];
        bd_addr[2] = FLASH_BD_ADDR[3];
        bd_addr[3] = FLASH_BD_ADDR[2];
        bd_addr[4] = FLASH_BD_ADDR[1];
        bd_addr[5] = FLASH_BD_ADDR[0];
        aci_hal_write_config_data_begin(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bd_addr);
    }
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_hal_write_config_data_end();

    // set Tx power level

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_hal_set_tx_power_level_begin(1, 5); // 1.4 dBm - same as LEGO firmware
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_hal_set_tx_power_level_end();

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    hci_le_read_local_version_begin();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    {
        uint8_t hci_version;
        uint16_t hci_revision;
        uint8_t lmp_pal_version;
        uint16_t manufacturer_name;
        uint16_t lmp_pal_subversion;
        hci_le_read_local_version_end(&hci_version, &hci_revision, &lmp_pal_version, &manufacturer_name, &lmp_pal_subversion);

        // seems like STM or LEGO hacked this command to get firmware version
        pbdrv_bluetooth_fw_version[0] = '0' + hci_revision;
        pbdrv_bluetooth_fw_version[1] = '.';
        pbdrv_bluetooth_fw_version[2] = '0' + (lmp_pal_subversion >> 4);
        pbdrv_bluetooth_fw_version[3] = '`' + (lmp_pal_subversion & 0xf);
        // LEGO firmware replaces (lmp_pal_subversion & 0xf) == 0 with "dev" but
        // it seems highly unlikely that we would ever see that
        pbdrv_bluetooth_fw_version[4] = '\0';
    }

    // init GATT layer

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_init_begin();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_gatt_init_end();

    // init GAP layer

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gap_init_begin(GAP_PERIPHERAL_ROLE | GAP_CENTRAL_ROLE, PRIVACY_DISABLED, sizeof(pbdrv_bluetooth_hub_name));
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gap_init_end(&gap_service_handle, &gap_dev_name_char_handle, &gap_appearance_char_handle);

    // set the device name

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_update_char_value_begin(gap_service_handle, gap_dev_name_char_handle,
        0, strlen(pbdrv_bluetooth_hub_name), pbdrv_bluetooth_hub_name);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // aci_gatt_update_char_value_end();

    // The chip always uses the same random address, so we have to generate
    // an actually random one to get a new address each time. This must be
    // called after aci_gap_init to take effect.

    // STM32F0 doesn't have a random number generator, so we use the bluetooth
    // chip to get some random bytes.
    hci_le_rand_begin();
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    {
        uint8_t rand_buf[8];
        hci_le_rand_end(rand_buf);

        // clear two msb to meet requirements of nonresolvable private address
        rand_buf[5] &= 0x3F;
        hci_le_set_random_address_begin(rand_buf);
    }
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    // hci_le_set_random_address_end();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_error_t init_uart_service(pbio_os_state_t *state, void *context) {
    // using well-known (but not standard) Nordic UART UUIDs

    // 6e400001-b5a3-f393-e0a9-e50e24dcca9e
    static const uint8_t nrf_uart_service_uuid[] = {
        0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
        0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e
    };
    // 6e400002-b5a3-f393-e0a9-e50e24dcca9e
    static const uint8_t nrf_uart_rx_char_uuid[] = {
        0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
        0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e
    };
    // 6e400003-b5a3-f393-e0a9-e50e24dcca9e
    static const uint8_t nrf_uart_tx_char_uuid[] = {
        0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
        0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e
    };

    PBIO_OS_ASYNC_BEGIN(state);

    // add the Nordic UART service (inspired by Add_Sample_Service() from
    // sample_service.c in BlueNRG vendor sample code and Adafruit config file
    // https://github.com/adafruit/Adafruit_nRF8001/blob/master/utility/uart/UART_over_BLE.xml)

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_serv_begin(UUID_TYPE_128, nrf_uart_service_uuid, PRIMARY_SERVICE, 7);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_serv_end(&uart_service_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_char_begin(uart_service_handle, UUID_TYPE_128, nrf_uart_rx_char_uuid,
        NUS_CHAR_SIZE, CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE,
        GATT_NOTIFY_ATTRIBUTE_WRITE, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_char_end(&uart_rx_char_handle);

    PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
    aci_gatt_add_char_begin(uart_service_handle, UUID_TYPE_128, nrf_uart_tx_char_uuid,
        NUS_CHAR_SIZE, CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PBIO_OS_AWAIT_UNTIL(state, hci_command_complete);
    aci_gatt_add_char_end(&uart_tx_char_handle);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_bluetooth_controller_reset_hard(void) {
    pybricks_notify_en = uart_tx_notify_en = false;
    conn_handle = peripheral_singleton.con_handle = 0;
    spi_disable_cs();
    bluetooth_reset(true);
}

pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    PBIO_OS_ASYNC_BEGIN(state);

    // Disconnect gracefully if connected to host.
    if (conn_handle) {
        PBIO_OS_AWAIT_WHILE(state, write_xfer_size);
        aci_gap_terminate_begin(conn_handle, HCI_OE_USER_ENDED_CONNECTION);
        PBIO_OS_AWAIT_UNTIL(state, hci_command_status);
        aci_gap_terminate_end();
        PBIO_OS_AWAIT_UNTIL(state, conn_handle == 0);
    }

    pbdrv_bluetooth_controller_reset_hard();

    PBIO_OS_AWAIT_MS(state, timer, 50);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer) {

    static pbio_os_state_t sub;
    static size_t idx;

    PBIO_OS_ASYNC_BEGIN(state);

    // take Bluetooth chip out of reset
    bluetooth_reset(false);

    // wait for the Bluetooth chip to send the reset reason event so we know
    // that it is ready to receive commands
    reset_reason = 0;
    PBIO_OS_AWAIT_UNTIL(state, reset_reason);

    // Init hci, gatt, gap, and services.
    static const pbio_os_process_func_t init_funcs[] = {
        hci_init,
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

    static pbio_os_state_t main_thread_state;
    static pbio_os_state_t sub;

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

            // - spi_irq is set by the interrupt handler when there is new
            //   data to be processed, which also polls this process in order
            //   to push PROCESS_WAIT_UNTIL to evaluate this condition again.
            // - write_xfer_size is set when main thread
            //   driven above above wants to send something. This doesn't poll
            //   the process because we are already here.
            spi_irq || write_xfer_size;
        });

        // if there is a pending read message
        if (spi_irq) {
            PBIO_OS_AWAIT(state, &sub, spi_read(&sub));
        }
        // if there is a pending write message
        else if (write_xfer_size) {
            PBIO_OS_AWAIT(state, &sub, spi_write(&sub));
        }
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

void pbdrv_bluetooth_init_hci(void) {
    bluetooth_reset(true);
    spi_init();
    pbio_os_process_start(&pbdrv_bluetooth_spi_process, pbdrv_bluetooth_spi_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLUETOOTH_STM32_BLUENRG
