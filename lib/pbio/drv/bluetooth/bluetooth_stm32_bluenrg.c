// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// Bluetooth for STM32 MCU with STMicro BlueNRG-MS

// This file hard-codes hardware access to save code size instead of using
// platform.c since this is only used on the BOOST Move hub.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_STM32_BLUENRG

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
#include <stm32f070xb.h>

#include <bluenrg_aci.h>
#include <bluenrg_gap.h>
#include <hci_le.h>
#include <hci_tl.h>

// hub name goes in special section so that it can be modified when flashing firmware
__attribute__((section(".name")))
char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

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
volatile bool spi_irq = false;
// set to false when xfer is started and true when xfer is complete
volatile bool spi_xfer_complete = false;

// set to false when hci command is started and true when command is completed
static bool hci_command_complete = false;
// handle to connected Bluetooth device
static uint16_t conn_handle;

// Pybricks GATT service handles
static uint16_t pybricks_service_handle, pybricks_char_handle;

// Nordic UART GATT service handles
static uint16_t uart_service_handle, uart_rx_char_handle, uart_tx_char_handle;

PROCESS(pbdrv_bluetooth_spi_process, "Bluetooth SPI");

LIST(task_queue);

static bool bluetooth_ready;
static bool pybricks_notify_en;
static bool uart_tx_notify_en;
static pbdrv_bluetooth_on_event_t bluetooth_on_event;
static pbdrv_bluetooth_receive_handler_t receive_handler;

static const pbdrv_gpio_t reset_gpio = { .bank = GPIOB, .pin = 6 };
static const pbdrv_gpio_t cs_gpio = { .bank = GPIOB, .pin = 12 };
static const pbdrv_gpio_t irq_gpio = { .bank = GPIOD, .pin = 2 };
static const pbdrv_gpio_t mosi_gpio = { .bank = GPIOC, .pin = 3 };
static const pbdrv_gpio_t miso_gpio = { .bank = GPIOC, .pin = 2 };
static const pbdrv_gpio_t sck_gpio = { .bank = GPIOB, .pin = 13 };

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

void pbdrv_bluetooth_init(void) {
    bluetooth_reset(true);
    spi_init();
}

void pbdrv_bluetooth_power_on(bool on) {
    if (on) {
        process_start(&pbdrv_bluetooth_spi_process, NULL);
    } else {
        // REVISIT: should probably gracefully shutdown in case we are in the
        // middle of something
        process_exit(&pbdrv_bluetooth_spi_process);
    }
}

bool pbdrv_bluetooth_is_ready(void) {
    return bluetooth_ready;
}

/**
 * Sets advertising data and enables advertisements.
 */
static PT_THREAD(set_discoverable(struct pt *pt, pbio_task_t *task)) {
    // c5f50001-8280-46da-89f4-6d8051e4aeef
    static const uint8_t service_uuids[] = {
        AD_TYPE_128_BIT_SERV_UUID,
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
        0xda, 0x46, 0x80, 0x82, 0x01, 0x00, 0xf5, 0xc5 // Pybricks service UUID
    };

    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
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
    PT_WAIT_UNTIL(pt, hci_command_complete);
    hci_le_set_scan_response_data_end();

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gap_set_discoverable_begin(ADV_IND, 0, 0, PUBLIC_ADDR, NO_WHITE_LIST_USE,
        0, NULL, sizeof(service_uuids), service_uuids, 0, 0);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gap_set_discoverable_end();

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

void pbdrv_bluetooth_start_advertising(void) {
    static pbio_task_t task;
    pbio_task_init(&task, set_discoverable, NULL);
    pbio_task_start(task_queue, &task);
}

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
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

void pbdrv_bluetooth_set_on_event(pbdrv_bluetooth_on_event_t on_event) {
    bluetooth_on_event = on_event;
}

static PT_THREAD(send_value_notification(struct pt *pt, pbio_task_t *task))
{
    pbdrv_bluetooth_send_context_t *send = task->context;

    PT_BEGIN(pt);

retry:
    PT_WAIT_WHILE(pt, write_xfer_size);

    uint16_t service_handle, attr_handle;
    if (send->connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS) {
        if (!pybricks_notify_en) {
            goto done;
        }
        service_handle = pybricks_service_handle;
        attr_handle = pybricks_char_handle;
    } else if (send->connection == PBDRV_BLUETOOTH_CONNECTION_UART) {
        if (!uart_tx_notify_en) {
            goto done;
        }
        service_handle = uart_service_handle;
        attr_handle = uart_tx_char_handle;
    } else {
        // called with invalid connection type
        assert(0);
        task->status = PBIO_ERROR_INVALID_ARG;
        goto done;
    }

    aci_gatt_update_char_value_begin(service_handle, attr_handle, 0, send->size, send->data);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    tBleStatus ret = aci_gatt_update_char_value_end();

    if (ret == BLE_STATUS_INSUFFICIENT_RESOURCES) {
        // this will happen if notifications are enabled and the previous
        // changes haven't been sent over the air yet
        goto retry;
    }

done:
    send->done();

    task->status = PBIO_SUCCESS;

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
        process_poll(&pbdrv_bluetooth_spi_process);
    }
}

// overrides weak function in startup_*.S
void EXTI2_3_IRQHandler(void) {
    spi_irq = !!(GPIOD->IDR & GPIO_IDR_2);

    // clear the interrupt
    EXTI->PR = EXTI_PR_PR2;

    process_poll(&pbdrv_bluetooth_spi_process);
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

static PT_THREAD(init_device_information_service(struct pt *pt)) {
    static const uint8_t device_information_service_uuid[] = { 0x0A, 0x18 }; // 0x180A
    static const uint8_t firmware_version_char_uuid[] = { 0x26, 0x2A }; // 0x2A26
    static const uint8_t software_version_char_uuid[] = { 0x28, 0x2A }; // 0x2A28

    static uint16_t service_handle, fw_ver_char_handle, sw_ver_char_handle, pnp_id_char_handle;

    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_serv_begin(UUID_TYPE_16, device_information_service_uuid, PRIMARY_SERVICE, 7);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_serv_end(&service_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(service_handle, UUID_TYPE_16, firmware_version_char_uuid,
        sizeof(PBIO_VERSION_STR) - 1, CHAR_PROP_READ, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_CONSTANT);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_char_end(&fw_ver_char_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_update_char_value_begin(service_handle, fw_ver_char_handle,
        0, sizeof(PBIO_VERSION_STR) - 1, PBIO_VERSION_STR);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_update_char_value_end();

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(service_handle, UUID_TYPE_16, software_version_char_uuid,
        sizeof(PBIO_PROTOCOL_VERSION_STR) - 1, CHAR_PROP_READ, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_CONSTANT);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_char_end(&sw_ver_char_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_update_char_value_begin(service_handle, sw_ver_char_handle,
        0, sizeof(PBIO_PROTOCOL_VERSION_STR) - 1, PBIO_PROTOCOL_VERSION_STR);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_update_char_value_end();

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(service_handle, UUID_TYPE_16, (const uint8_t *)PNP_ID,
        sizeof(PNP_ID) - 3, CHAR_PROP_READ, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_CONSTANT);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_char_end(&pnp_id_char_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_update_char_value_begin(service_handle, pnp_id_char_handle,
        0, sizeof(PNP_ID) - 3, &PNP_ID[2]);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_update_char_value_end();

    PT_END(pt);
}

static PT_THREAD(init_pybricks_service(struct pt *pt)) {
    // c5f50001-8280-46da-89f4-6d8051e4aeef
    static const uint8_t pybricks_service_uuid[] = {
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
        0xda, 0x46, 0x80, 0x82, 0x01, 0x00, 0xf5, 0xc5
    };

    // c5f50002-8280-46da-89f4-6d8051e4aeef
    static const uint8_t pybricks_char_uuid[] = {
        0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
        0xda, 0x46, 0x80, 0x82, 0x02, 0x00, 0xf5, 0xc5
    };

    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_serv_begin(UUID_TYPE_128, pybricks_service_uuid, PRIMARY_SERVICE, 4);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_serv_end(&pybricks_service_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(pybricks_service_handle, UUID_TYPE_128, pybricks_char_uuid,
        NUS_CHAR_SIZE, CHAR_PROP_WRITE_WITHOUT_RESP | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE,
        GATT_NOTIFY_ATTRIBUTE_WRITE, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_char_end(&pybricks_char_handle);

    PT_END(pt);
}

// processes an event received from the Bluetooth chip
static void handle_event(hci_event_pckt *event) {
    switch (event->evt) {
        case EVT_DISCONN_COMPLETE: {
            evt_disconn_complete *evt = (evt_disconn_complete *)event->data;
            if (conn_handle == evt->handle) {
                conn_handle = 0;
                pybricks_notify_en = false;
                uart_tx_notify_en = false;
            }
        }
        break;
        case EVT_CMD_COMPLETE:
            hci_command_complete = true;
            break;
        case EVT_LE_META_EVENT: {
            evt_le_meta_event *evt = (evt_le_meta_event *)event->data;
            switch (evt->subevent) {
                case EVT_LE_CONN_COMPLETE: {
                    evt_le_connection_complete *subevt = (evt_le_connection_complete *)evt->data;
                    conn_handle = subevt->handle;
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
                    if (subevt->attr_handle == pybricks_char_handle + 1) {
                        if (receive_handler) {
                            receive_handler(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS, subevt->att_data, subevt->data_length);
                        }
                    } else if (subevt->attr_handle == pybricks_char_handle + 2) {
                        pybricks_notify_en = subevt->att_data[0];
                    } else if (subevt->attr_handle == uart_rx_char_handle + 1) {
                        if (receive_handler) {
                            receive_handler(PBDRV_BLUETOOTH_CONNECTION_UART, subevt->att_data, subevt->data_length);
                        }
                    } else if (subevt->attr_handle == uart_tx_char_handle + 2) {
                        uart_tx_notify_en = subevt->att_data[0];
                    }
                }
                break;
            }
        }
        break;
    }

    if (bluetooth_on_event) {
        bluetooth_on_event();
    }
}

// read message from BlueNRG chip
static PT_THREAD(spi_read(struct pt *pt)) {
    hci_uart_pckt *pckt = (hci_uart_pckt *)read_buf;
    uint8_t wbuf, rbuf;

    PT_BEGIN(pt);

retry:
    if (!spi_irq) {
        // if SPI_IRQ went away, reading will fail, so don't try
        goto end;
    }

    spi_enable_cs();

    // send the read header
    spi_start_xfer(read_header_tx, header_rx, BLUENRG_HEADER_SIZE);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    // keep retrying until the chip is ready to read
    if (!get_bluenrg_buf_size(&wbuf, &rbuf) || rbuf == 0) {
        // TODO: should probably have a timeout (and reset the chip after that?)
        spi_disable_cs();
        goto retry;
    }

    // read the message
    spi_start_xfer(dummy_write_buf, read_buf, rbuf);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    spi_disable_cs();

    // handle the received data (note: can't use switch statement here because
    // we are using protothreads)
    if (pckt->type == HCI_EVENT_PKT) {
        handle_event((hci_event_pckt *)pckt->data);
    }
    // TODO: do we need to handle ACL packets (HCI_ACLDATA_PKT)?

end:;
    PT_END(pt);
}

// write message to BlueNRG chip
static PT_THREAD(spi_write(struct pt *pt)) {
    uint8_t wbuf, rbuf;

    PT_BEGIN(pt);

retry:
    spi_enable_cs();

    // send the write header
    spi_start_xfer(write_header_tx, header_rx, BLUENRG_HEADER_SIZE);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    // keep retrying until the chip is ready to write
    if (!get_bluenrg_buf_size(&wbuf, &rbuf) || wbuf < write_xfer_size) {
        // TODO: should probably have a timeout (and reset the chip after that?)
        spi_disable_cs();
        goto retry;
    }

    // write the message
    spi_start_xfer(write_buf, dummy_read_buf, write_xfer_size);
    PT_WAIT_UNTIL(pt, spi_xfer_complete);

    spi_disable_cs();

    // set to 0 to indicate that xfer is complete
    write_xfer_size = 0;

    PT_END(pt);
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

    hci_command_complete = false;
    process_poll(&pbdrv_bluetooth_spi_process);
}

// implements function for BlueNRG library
void hci_recv_resp(struct hci_response *r) {
    // TODO: might be a good idea to make sure opcodes match
    memcpy(r->rparam, &read_buf[HCI_HDR_SIZE + HCI_EVENT_HDR_SIZE + EVT_CMD_COMPLETE_SIZE], r->rlen);
}

// Initializes the Bluetooth chip
// this function is largely inspired by the LEGO bootloader
static PT_THREAD(hci_init(struct pt *pt)) {
    static const uint8_t mode = 2; // Slave and master; Only one connection; 12 KB of RAM retention
    static uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
    uint8_t bd_addr[6];

    PT_BEGIN(pt);

    // set the mode

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_hal_write_config_data_begin(CONFIG_DATA_MODE_OFFSET, CONFIG_DATA_MODE_LEN, &mode);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_hal_write_config_data_end();

    // set the Bluetooth address

    PT_WAIT_WHILE(pt, write_xfer_size);
    // has to be in little-endian format, but stored in big-endian in flash
    bd_addr[0] = FLASH_BD_ADDR[5];
    bd_addr[1] = FLASH_BD_ADDR[4];
    bd_addr[2] = FLASH_BD_ADDR[3];
    bd_addr[3] = FLASH_BD_ADDR[2];
    bd_addr[4] = FLASH_BD_ADDR[1];
    bd_addr[5] = FLASH_BD_ADDR[0];
    aci_hal_write_config_data_begin(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bd_addr);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_hal_write_config_data_end();

    // set Tx power level

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_hal_set_tx_power_level_begin(1, 5); // 1.4 dBm - same as LEGO firmware
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_hal_set_tx_power_level_end();

    // init GATT layer

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_init_begin();
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_init_end();

    // init GAP layer

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gap_init_begin(GAP_PERIPHERAL_ROLE, PRIVACY_DISABLED, 16); // 16 comes from LEGO bootloader
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gap_init_end(&gap_service_handle, &gap_dev_name_char_handle, &gap_appearance_char_handle);

    // set the device name

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_update_char_value_begin(gap_service_handle, gap_dev_name_char_handle,
        0, strlen(pbdrv_bluetooth_hub_name), pbdrv_bluetooth_hub_name);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_update_char_value_end();

    PT_END(pt);
}

static PT_THREAD(init_uart_service(struct pt *pt)) {
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

    PT_BEGIN(pt);

    // add the Nordic UART service (inspired by Add_Sample_Service() from
    // sample_service.c in BlueNRG vendor sample code and Adafruit config file
    // https://github.com/adafruit/Adafruit_nRF8001/blob/master/utility/uart/UART_over_BLE.xml)

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_serv_begin(UUID_TYPE_128, nrf_uart_service_uuid, PRIMARY_SERVICE, 7);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_serv_end(&uart_service_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(uart_service_handle, UUID_TYPE_128, nrf_uart_rx_char_uuid,
        NUS_CHAR_SIZE, CHAR_PROP_WRITE_WITHOUT_RESP, ATTR_PERMISSION_NONE,
        GATT_NOTIFY_ATTRIBUTE_WRITE, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_char_end(&uart_rx_char_handle);

    PT_WAIT_WHILE(pt, write_xfer_size);
    aci_gatt_add_char_begin(uart_service_handle, UUID_TYPE_128, nrf_uart_tx_char_uuid,
        NUS_CHAR_SIZE, CHAR_PROP_NOTIFY, ATTR_PERMISSION_NONE,
        GATT_DONT_NOTIFY_EVENTS, MIN_ENCRY_KEY_SIZE, CHAR_VALUE_LEN_VARIABLE);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    aci_gatt_add_char_end(&uart_tx_char_handle);

    PT_END(pt);
}

static PT_THREAD(init_task(struct pt *pt, pbio_task_t *task)) {
    static struct pt child_pt;

    PT_BEGIN(pt);

    // wait for the Bluetooth chip to send the reset reason event so we know
    // that it is ready to receive commands
    reset_reason = 0;
    PT_WAIT_UNTIL(pt, reset_reason);

    PT_SPAWN(pt, &child_pt, hci_init(&child_pt));
    PT_SPAWN(pt, &child_pt, init_device_information_service(&child_pt));
    PT_SPAWN(pt, &child_pt, init_pybricks_service(&child_pt));
    PT_SPAWN(pt, &child_pt, init_uart_service(&child_pt));
    bluetooth_ready = true;
    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

PROCESS_THREAD(pbdrv_bluetooth_spi_process, ev, data) {
    static struct pt child_pt;

    PROCESS_EXITHANDLER({
        spi_disable_cs();
        bluetooth_reset(true);
        bluetooth_ready = pybricks_notify_en = uart_tx_notify_en = false;
        conn_handle = 0;
        PROCESS_EXIT();
    });

    PROCESS_BEGIN();

    // take Bluetooth chip out of reset
    bluetooth_reset(false);

    static pbio_task_t task;
    pbio_task_init(&task, init_task, NULL);
    pbio_task_start(task_queue, &task);

    while (true) {
        PROCESS_WAIT_UNTIL(spi_irq || write_xfer_size);
        // if there is a pending read message
        if (spi_irq) {
            PROCESS_PT_SPAWN(&child_pt, spi_read(&child_pt));
        }
        // if there is a pending write message
        else if (write_xfer_size) {
            PROCESS_PT_SPAWN(&child_pt, spi_write(&child_pt));
        }

        pbio_task_queue_run_once(task_queue);
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_BLUETOOTH_STM32_BLUENRG
