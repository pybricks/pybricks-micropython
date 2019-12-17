// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner

// Bluetooth for STM32 MCU with TI CC2640

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/event.h>
#include <pbio/util.h>
#include <pbsys/sys.h>

#include "sys/autostart.h"
#include "sys/etimer.h"
#include "sys/process.h"
#include "sys/pt.h"
#include "../../src/processes.h"

#include <stm32l4xx_hal.h>

#include <att.h>
#include <gap.h>
#include <gatt.h>
#include <gattservapp.h>
#include <hci_ext.h>
#include <hci_tl.h>
#include <hci.h>
#include <util.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define DBG(fmt, ...) { \
    char dbg[64]; \
    snprintf(dbg, 64, fmt "\r\n", ##__VA_ARGS__); \
    for (char *d = dbg; *d; d++) { \
        while (!(LPUART1->ISR & USART_ISR_TXE)) { } \
        LPUART1->TDR = *d; \
    } \
}
#else
#define DBG(...)
#endif

// name used for standard GAP device name characteristic
#define DEV_NAME "Pybricks Hub"

// bluetooth address is set at factory at this address
#define FLASH_BD_ADDR ((const uint8_t *)0x08007ff0)

// TI Network Processor Interface (NPI)
#define NPI_SPI_SOF             0xFE    // start of frame
#define NPI_SPI_HEADER_LEN      3       // zero pad, SOF, length

#define NO_CONNECTION           0xFFFF

// max data size for nRF UART characteristics
#define NRF_CHAR_SIZE 20


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
// handle to connected Bluetooth device
static uint16_t conn_handle = NO_CONNECTION;

// GATT service handles
static uint16_t gatt_service_handle, gatt_service_end_handle;
// GAP service handles
static uint16_t gap_service_handle, gap_service_end_handle;
// Pybricks service handles
static uint16_t pybricks_service_handle, pybricks_service_end_handle, pybricks_char_handle;
// nRF UART service handles
static uint16_t uart_service_handle, uart_service_end_handle, uart_rx_char_handle, uart_tx_char_handle;
// nRF UART tx notifications enabled
static bool uart_tx_notify_en;
// nRF UART tx is in progress
static bool uart_tx_busy;
// buffer to queue UART tx data
static uint8_t uart_tx_buf[NRF_CHAR_SIZE];
// bytes used in uart_tx_buf
static uint8_t uart_tx_buf_size;

// 6e400001-b5a3-f393-e0a-9e50e24dcca9e
static const uint8_t pybricks_service_uuid[] = {
    0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
    0xda, 0x46, 0x80, 0x82, 0x01, 0x00, 0xf5, 0xc5
};

// c5f50002-8280-46da-89f4-6d8051e4aeef
static const uint8_t pybricks_char_uuid[] = {
    0xef, 0xae, 0xe4, 0x51, 0x80, 0x6d, 0xf4, 0x89,
    0xda, 0x46, 0x80, 0x82, 0x02, 0x00, 0xf5, 0xc5
};

// using well-known (but not standard) nRF UART UUIDs

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

PROCESS(pbdrv_bluetooth_hci_process, "Bluetooth HCI");
PROCESS(pbdrv_bluetooth_spi_process, "Bluetooth SPI");

static SPI_HandleTypeDef bt_spi;

typedef enum {
    RESET_STATE_OUT_HIGH,   // in reset
    RESET_STATE_OUT_LOW,    // out of reset
    RESET_STATE_INPUT,      // ?
} reset_state_t;

static void bluetooth_reset(reset_state_t reset) {
    GPIO_InitTypeDef gpio_init = { 0 };

    // Implied defaults: no pull, low speed, alternate function 0

    // nRESET
    gpio_init.Pin = GPIO_PIN_2;
    gpio_init.Mode = reset == RESET_STATE_INPUT ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOD, &gpio_init);
    if (reset != RESET_STATE_INPUT) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, reset == RESET_STATE_OUT_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

static void bluetooth_init() {
    bluetooth_reset(RESET_STATE_OUT_LOW);
}

static void spi_init() {
    GPIO_InitTypeDef gpio_init = { 0 };

    // Implied defaults: no pull, low speed, alternate function 0

    // nMRDY
    gpio_init.Pin = GPIO_PIN_15;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &gpio_init);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);

    // nSRDY
    gpio_init.Pin = GPIO_PIN_13;
    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &gpio_init);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    bt_spi.Instance = SPI1;
    bt_spi.Init.Mode = SPI_MODE_MASTER;
    bt_spi.Init.Direction = SPI_DIRECTION_2LINES;
    bt_spi.Init.DataSize = SPI_DATASIZE_8BIT;
    bt_spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    bt_spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    bt_spi.Init.NSS = SPI_NSS_SOFT;
    bt_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    bt_spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    bt_spi.Init.TIMode = SPI_TIMODE_DISABLE;
    bt_spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    bt_spi.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    HAL_SPI_Init(&bt_spi);
}

// TODO: move this stuff to platform.c

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    GPIO_InitTypeDef gpio_init;
    static DMA_HandleTypeDef rx_dma, tx_dma;

    gpio_init.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_PULLUP;
    gpio_init.Alternate = GPIO_AF5_SPI1;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // TODO: verify this against SPI clock speed
    HAL_GPIO_Init(GPIOB, &gpio_init);

    tx_dma.Instance = DMA1_Channel3;
    tx_dma.Init.Request = DMA_REQUEST_1;
    tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    tx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    tx_dma.Init.MemInc = DMA_MINC_ENABLE;
    tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    tx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    tx_dma.Init.Mode = DMA_NORMAL;
    tx_dma.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&tx_dma);
    __HAL_LINKDMA(hspi, hdmatx, tx_dma);

    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    rx_dma.Instance = DMA1_Channel2;
    rx_dma.Init.Request = DMA_REQUEST_1;
    rx_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    rx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    rx_dma.Init.MemInc = DMA_MINC_ENABLE;
    rx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    rx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    rx_dma.Init.Mode = DMA_NORMAL;
    rx_dma.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&rx_dma);
    __HAL_LINKDMA(hspi, hdmarx, rx_dma);

    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
    HAL_NVIC_DisableIRQ(DMA1_Channel2_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Channel3_IRQn);
    HAL_DMA_DeInit(hspi->hdmarx);
    HAL_DMA_DeInit(hspi->hdmatx);
}

void DMA1_Channel2_IRQHandler(void) {
    HAL_DMA_IRQHandler(bt_spi.hdmarx);
}

void DMA1_Channel3_IRQHandler(void) {
    HAL_DMA_IRQHandler(bt_spi.hdmatx);
}

void EXTI15_10_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
}

// end platform code

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    spi_srdy = !HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
    process_poll(&pbdrv_bluetooth_spi_process);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    spi_xfer_complete = true;
    process_poll(&pbdrv_bluetooth_spi_process);
}

#if PBIO_CONFIG_ENABLE_DEINIT
static void bluetooth_deinit() {
    bluetooth_reset(RESET_STATE_OUT_LOW);
}

static void spi_deinit() {
    NVIC_DisableIRQ(EXTI15_10_IRQn);

    HAL_SPI_Abort(&bt_spi);
    HAL_SPI_DeInit(&bt_spi);
}
#else // PBIO_CONFIG_ENABLE_DEINIT
#undef PROCESS_EXITHANDLER
#define PROCESS_EXITHANDLER(x)
#endif // PBIO_CONFIG_ENABLE_DEINIT

/**
 * Sets the MRDY signal.
 */
static void spi_set_mrdy(bool mrdy) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, mrdy ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

// static void pybricks_char_modified(uint8_t *data, uint8_t size) {
//     if (size < 2) {
//         return;
//     }

//     switch (data[0]) {
//     case PBIO_COM_MSG_TYPE_CMD:
//         process_post(&pbsys_process, PBIO_EVENT_COM_CMD, (process_data_t)(uint32_t)data[1]);
//         break;
//     }
// }

static void uart_rx_char_modified(uint8_t *data, uint8_t size);

// processes an event received from the Bluetooth chip
static void handle_event(uint8_t *packet) {
    uint8_t event = packet[0];
    uint8_t size = packet[1];
    uint8_t *data = &packet[2];

    UNUSED(size);

    switch (event) {
    case HCI_COMMAND_COMPLETE_EVENT:
        hci_command_complete = true;
        break;

    case HCI_LE_EXT_EVENT:
        {
            uint16_t event_code = (data[1] << 8) | data[0];
            HCI_StatusCodes_t status = data[2];
            uint16_t connection_handle = (data[4] << 8) | data[3];
            uint8_t pdu_len = data[5];

            UNUSED(status);
            UNUSED(pdu_len);

            switch (event_code) {
            case ATT_EVENT_EXCHANGE_MTU_REQ:
                {
                    attExchangeMTURsp_t rsp;

                    rsp.serverRxMTU = 158;
                    ATT_ExchangeMTURsp(connection_handle, &rsp);
                }
                break;
            case ATT_EVENT_READ_BY_TYPE_REQ:
                {
                    uint16_t start_handle = (data[7] << 8) | data[6];
                    uint16_t end_handle = (data[9] << 8) | data[8];
                    uint16_t type = (data[11] << 8) | data[10];

                    UNUSED(end_handle);

                    DBG("s %04X t %04X", start_handle, type);
                    switch (type) {
                    case GATT_CHARACTER_UUID:
                        if (start_handle <= gatt_service_handle) {
                            uint16_t handle = gatt_service_handle + 1;
                            attReadByTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = handle & 0xFF;
                            buf[1] = (handle >> 8) & 0xFF;
                            buf[2] = GATT_PROP_INDICATE;
                            buf[3] = ++handle & 0xFF;
                            buf[4] = (handle >> 8) & 0xFF;
                            buf[5] = SERVICE_CHANGED_UUID & 0xFF;
                            buf[6] = (SERVICE_CHANGED_UUID >> 8) & 0xFF;

                            rsp.pDataList = buf;
                            rsp.dataLen = 7;
                            ATT_ReadByTypeRsp(connection_handle, &rsp);
                        }
                        else if (start_handle <= gap_service_handle) {
                            uint16_t handle = gap_service_handle + 1;
                            attReadByTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = handle & 0xFF;
                            buf[1] = (handle >> 8) & 0xFF;
                            buf[2] = GATT_PROP_READ;
                            buf[3] = ++handle & 0xFF;
                            buf[4] = (handle >> 8) & 0xFF;
                            buf[5] = DEVICE_NAME_UUID & 0xFF;
                            buf[6] = (DEVICE_NAME_UUID >> 8) & 0xFF;

                            rsp.pDataList = buf;
                            rsp.dataLen = 7;
                            ATT_ReadByTypeRsp(connection_handle, &rsp);
                        }
                        else if (start_handle <= pybricks_service_handle) {
                            uint16_t handle = pybricks_service_handle + 1;
                            attReadByTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = handle & 0xFF;
                            buf[1] = (handle >> 8) & 0xFF;
                            buf[2] = GATT_PROP_READ | GATT_PROP_WRITE |
                                               GATT_PROP_WRITE_NO_RSP | GATT_PROP_NOTIFY;
                            buf[3] = ++handle & 0xFF;
                            buf[4] = (handle >> 8) & 0xFF;
                            memcpy(&buf[5], pybricks_char_uuid, 16);

                            rsp.pDataList = buf;
                            rsp.dataLen = 21;
                            ATT_ReadByTypeRsp(connection_handle, &rsp);
                        }
                        else if (start_handle <= uart_service_handle) {
                            uint16_t handle = uart_service_handle + 1;
                            attReadByTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = handle & 0xFF;
                            buf[1] = (handle >> 8) & 0xFF;
                            buf[2] = GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
                            buf[3] = ++handle & 0xFF;
                            buf[4] = (handle >> 8) & 0xFF;
                            memcpy(&buf[5], nrf_uart_rx_char_uuid, 16);

                            rsp.pDataList = buf;
                            rsp.dataLen = 21;
                            ATT_ReadByTypeRsp(connection_handle, &rsp);
                        }
                        else if (start_handle <= uart_service_handle + 2) {
                            uint16_t handle = uart_service_handle + 3;
                            attReadByTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = handle & 0xFF;
                            buf[1] = (handle >> 8) & 0xFF;
                            buf[2] = GATT_PROP_NOTIFY;
                            buf[3] = ++handle & 0xFF;
                            buf[4] = (handle >> 8) & 0xFF;
                            memcpy(&buf[5], nrf_uart_tx_char_uuid, 16);

                            rsp.pDataList = buf;
                            rsp.dataLen = 21;
                            ATT_ReadByTypeRsp(connection_handle, &rsp);
                        }
                        else {
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

            case ATT_EVENT_READ_REQ:
                {
                    uint16_t handle = (data[7] << 8) | data[6];

                    if (handle == gap_service_handle + 2) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE-1];

                        memcpy(&buf[0], DEV_NAME, sizeof(DEV_NAME));
                        rsp.len = sizeof(DEV_NAME) - 1;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    }
                    else if (handle == gap_service_handle + 4) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE-1];

                        buf[0] = GAP_APPEARE_UNKNOWN & 0xFF;
                        buf[1] = (GAP_APPEARE_UNKNOWN >> 8) & 0xFF;
                        rsp.len = 2;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    }
                    else if (handle == gap_service_handle + 6) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE-1];

                        // FIXME: what should these values be?
                        buf[0] = 0xFFFF & 0xFF; // intervalMin
                        buf[1] = (0xFFFF >> 8) & 0xFF;
                        buf[2] = 0xFFFF & 0xFF; // intervalMax
                        buf[3] = (0xFFFF >> 8) & 0xFF;
                        buf[4] = 0xFFFF & 0xFF; // latency
                        buf[5] = (0xFFFF >> 8) & 0xFF;
                        buf[6] = 0xFFFF & 0xFF; // timeout
                        buf[7] = (0xFFFF >> 8) & 0xFF;
                        rsp.len = 8;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    }
                    else if (handle == uart_tx_char_handle + 1) {
                        attReadRsp_t rsp;
                        uint8_t buf[ATT_MTU_SIZE-1];

                        buf[0] = uart_tx_notify_en;
                        buf[1] = 0;
                        rsp.len = 2;
                        rsp.pValue = buf;
                        ATT_ReadRsp(connection_handle, &rsp);
                    }
                    else {
                        DBG("unhandled read req: %04X", handle);
                    }
                }
                break;

            case ATT_EVENT_READ_BY_GRP_TYPE_REQ:
                {
                    uint16_t start_handle = (data[7] << 8) | data[6];
                    uint16_t end_handle = (data[9] << 8) | data[8];
                    uint16_t group_type = (data[11] << 8) | data[10];

                    UNUSED(end_handle);

                    DBG("s %04X g %04X", start_handle, group_type);
                    switch (group_type) {
                    case GATT_PRIMARY_SERVICE_UUID:
                        if (start_handle == gatt_service_handle) {
                            attReadByGrpTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = gatt_service_handle & 0xFF;
                            buf[1] = (gatt_service_handle >> 8) & 0xFF;
                            buf[2] = gatt_service_end_handle & 0xFF;
                            buf[3] = (gatt_service_end_handle >> 8) & 0xFF;
                            buf[4] = GATT_SERVICE_UUID & 0xFF;
                            buf[5] = (GATT_SERVICE_UUID >> 8) & 0xFF;

                            rsp.pDataList = buf;
                            rsp.dataLen = 6;
                            ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                        }
                        else if (start_handle == gap_service_handle) {
                            attReadByGrpTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = gap_service_handle & 0xFF;
                            buf[1] = (gap_service_handle >> 8) & 0xFF;
                            buf[2] = gap_service_end_handle & 0xFF;
                            buf[3] = (gap_service_end_handle >> 8) & 0xFF;
                            buf[4] = GAP_SERVICE_UUID & 0xFF;
                            buf[5] = (GAP_SERVICE_UUID >> 8) & 0xFF;

                            rsp.pDataList = buf;
                            rsp.dataLen = 6;
                            ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                        }
                        else if (start_handle == pybricks_service_handle) {
                            attReadByGrpTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = pybricks_service_handle & 0xFF;
                            buf[1] = (pybricks_service_handle >> 8) & 0xFF;
                            buf[2] = pybricks_service_end_handle & 0xFF;
                            buf[3] = (pybricks_service_end_handle >> 8) & 0xFF;
                            memcpy(&buf[4], pybricks_service_uuid, 16);

                            rsp.pDataList = buf;
                            rsp.dataLen = 20;
                            ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                        }
                        else if (start_handle == uart_service_handle) {
                            attReadByGrpTypeRsp_t rsp;
                            uint8_t buf[ATT_MTU_SIZE-2];

                            buf[0] = uart_service_handle & 0xFF;
                            buf[1] = (uart_service_handle >> 8) & 0xFF;
                            buf[2] = uart_service_end_handle & 0xFF;
                            buf[3] = (uart_service_end_handle >> 8) & 0xFF;
                            memcpy(&buf[4], nrf_uart_service_uuid, 16);

                            rsp.pDataList = buf;
                            rsp.dataLen = 20;
                            ATT_ReadByGrpTypeRsp(connection_handle, &rsp);
                        }
                        else {
                            attErrorRsp_t rsp;

                            rsp.reqOpcode = ATT_READ_BY_GRP_TYPE_REQ;
                            rsp.handle = start_handle;
                            rsp.errCode = ATT_ERR_INVALID_VALUE;
                            ATT_ErrorRsp(connection_handle, &rsp);
                        }
                        break;
                    default:
                        DBG("unhandled read by grp type req: %05X", group_type);
                        break;
                    }
                }
                break;

            case ATT_EVENT_WRITE_REQ:
                {
                    uint16_t char_handle = (data[9] << 8) | data[8];

                    DBG("w: %04X %04X %d", char_handle, uart_tx_char_handle, pdu_len - 4);
                    if (char_handle == uart_rx_char_handle) {
                        uart_rx_char_modified(&data[10], pdu_len - 4);
                    }
                    else if (char_handle == uart_tx_char_handle + 1) {
                        uart_tx_notify_en = data[10];
                        DBG("noti: %d", uart_tx_notify_en);
                    }
                    else {
                        DBG("unhandled write req: %04X", char_handle);
                    }
                    ATT_WriteRsp(connection_handle);
                }
                break;

            case GAP_LINK_ESTABLISHED:
                conn_handle = (data[11] << 8) | data[10];
                DBG("link: %04x", conn_handle);
                break;

            case GAP_LINK_TERMINATED:
                {
                    DBG("bye: %04x", connection_handle);
                    if (conn_handle == connection_handle) {
                        conn_handle = NO_CONNECTION;
                        uart_tx_notify_en = false;
                    }
                }
                break;

            case GAP_LINK_PARAM_UPDATE:
                // we get this event, but don't need to do anything about it
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

            case HCI_COMMAND_STATUS:
                {
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

    process_post_synch(&pbdrv_bluetooth_hci_process, PROCESS_EVENT_NONE, NULL);
}

// gets the number of bytes remaining to be read, if any
static bool get_npi_rx_size(uint8_t *rx_size) {
    if (read_buf[1] != NPI_SPI_SOF) {
        return false;
    }

    *rx_size = read_buf[2];

    return true;
}

PROCESS_THREAD(pbdrv_bluetooth_spi_process, ev, data) {
    static uint8_t read_xfer_size, xfer_size;

    PROCESS_EXITHANDLER(spi_deinit());

    PROCESS_BEGIN();

    spi_init();

    while (true) {
        PROCESS_WAIT_UNTIL(spi_srdy || write_xfer_size);

        spi_set_mrdy(true);

        // we can either read, write, or read and write at the same time

        if (write_xfer_size) {
            // if we are writing only, we have to wait until SRDY is asserted
            PROCESS_WAIT_UNTIL(spi_srdy);
        }
        else {
            // if we are reading only, the write buffer has to be all 0s
            memset(write_buf, 0, PBIO_ARRAY_SIZE(write_buf));
        }

        // send the write header
        spi_xfer_complete = false;
        HAL_SPI_TransmitReceive_DMA(&bt_spi, write_buf, read_buf, NPI_SPI_HEADER_LEN);
        PROCESS_WAIT_UNTIL(spi_xfer_complete);

        // Total transfer size is biggest of read and write sizes.
        read_xfer_size = 0;
        if (get_npi_rx_size(&read_xfer_size) && read_xfer_size > write_xfer_size - 4) {
            xfer_size = read_xfer_size + 4;
        }
        else {
            xfer_size = write_xfer_size;
        }

        // read the remaining message
        spi_xfer_complete = false;
        HAL_SPI_TransmitReceive_DMA(&bt_spi, &write_buf[NPI_SPI_HEADER_LEN],
                                    &read_buf[NPI_SPI_HEADER_LEN], xfer_size);
        PROCESS_WAIT_UNTIL(spi_xfer_complete);

        spi_set_mrdy(false);
        PROCESS_WAIT_UNTIL(!spi_srdy);

        if (write_xfer_size) {
            // set to 0 to indicate that xfer is complete
            write_xfer_size = 0;
            process_post_synch(&pbdrv_bluetooth_hci_process, PROCESS_EVENT_NONE, NULL);
        }

        if (read_xfer_size) {
            // handle the received data
            if (read_buf[NPI_SPI_HEADER_LEN] == HCI_EVENT_PACKET) {
                handle_event(&read_buf[NPI_SPI_HEADER_LEN + 1]);
            }
            // TODO: do we need to handle ACL packets (HCI_ACLDATA_PKT)?
        }
    }

    PROCESS_END();
}

// implements function for bt5stack library
HCI_StatusCodes_t HCI_sendHCICommand(uint16_t opcode, uint8_t *pData, uint8_t dataLength) {
    write_buf[0] = NPI_SPI_SOF;
    write_buf[1] = dataLength + 4;
    write_buf[2] = HCI_CMD_PACKET;
    write_buf[3] = opcode & 0xFF;
    write_buf[4] = opcode >> 8;
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
    process_post_synch(&pbdrv_bluetooth_spi_process, PROCESS_EVENT_NONE, NULL);

    return bleSUCCESS;
}

static PT_THREAD(gatt_init(struct pt *pt)) {
    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 4, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_AUTHOR_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(SERVICE_CHANGED_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CLIENT_CHAR_CFG_UUID, GATT_PERMIT_READ);
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
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_WRITE | GATT_PERMIT_AUTHEN_READ |
                                           GATT_PERMIT_AUTHEN_WRITE | GATT_PERMIT_ENCRYPT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(DEVICE_NAME_UUID, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_WRITE | GATT_PERMIT_AUTHEN_READ |
                                           GATT_PERMIT_AUTHEN_WRITE | GATT_PERMIT_ENCRYPT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(APPEARANCE_UUID, GATT_PERMIT_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_WRITE);
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
    HCI_EXT_setBdaddr(FLASH_BD_ADDR);
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
    GAP_deviceInit(GAP_PROFILE_PERIPHERAL, 8, NULL, NULL, 0);
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

static PT_THREAD(init_pybricks_service(struct pt *pt)) {
    PT_BEGIN(pt);

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 4, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_WRITE | GATT_PERMIT_AUTHEN_READ |
                                           GATT_PERMIT_AUTHEN_WRITE | GATT_PERMIT_AUTHOR_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(pybricks_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
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

static void uart_rx_char_modified(uint8_t *data, uint8_t size) {
    for (int i = 0; i < size; i++) {
        // TODO: set .port = bluetooth port
        pbio_event_uart_rx_data_t rx = { .byte = data[i] };
        process_post_synch(&pbsys_process, PBIO_EVENT_UART_RX, &rx);
    }
}

static PT_THREAD(init_uart_service(struct pt *pt)) {
    PT_BEGIN(pt);

    // add the nRF UART service (inspired by Add_Sample_Service() from
    // sample_service.c in BlueNRG vendor sample code and Adafruit config file
    // https://github.com/adafruit/Adafruit_nRF8001/blob/master/utility/uart/UART_over_BLE.xml)

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddService(GATT_PRIMARY_SERVICE_UUID, 6, GATT_MIN_ENCRYPT_KEY_SIZE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_WRITE | GATT_PERMIT_AUTHEN_READ |
                                           GATT_PERMIT_AUTHEN_WRITE | GATT_PERMIT_AUTHOR_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(nrf_uart_rx_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute(GATT_CHARACTER_UUID, GATT_PERMIT_WRITE | GATT_PERMIT_AUTHEN_READ |
                                           GATT_PERMIT_AUTHEN_WRITE | GATT_PERMIT_AUTHOR_READ);
    PT_WAIT_UNTIL(pt, hci_command_status);
    // ignoring response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    GATT_AddAttribute2(nrf_uart_tx_char_uuid, GATT_PERMIT_READ | GATT_PERMIT_WRITE);
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

static PT_THREAD(set_discoverable(struct pt *pt)) {
    uint8_t data[31];

    PT_BEGIN(pt);

    // Set advertising data

    PT_WAIT_WHILE(pt, write_xfer_size);
    data[0] = 2; // length
    data[1] = GAP_ADTYPE_FLAGS;
    data[2] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
    data[3] = 17; // length
    data[4] = GAP_ADTYPE_128BIT_MORE;
    memcpy(&data[5], pybricks_service_uuid, 16);
    data[21] = 2; // length
    data[22] = GAP_ADTYPE_POWER_LEVEL;
    data[23] = 0;
    GAP_updateAdvertistigData(GAP_AD_TYPE_ADVERTISEMNT_DATA, 24, data);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // Set scan response data

    PT_WAIT_WHILE(pt, write_xfer_size);
    data[0] = sizeof(DEV_NAME);  // same as 1 + strlen(DEV_NAME)
    data[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&data[2], DEV_NAME, sizeof(DEV_NAME));
    GAP_updateAdvertistigData(GAP_AD_TYPE_SCAN_RSP_DATA, sizeof(DEV_NAME) + 1, data);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    // make discoverable

    PT_WAIT_WHILE(pt, write_xfer_size);
    GAP_makeDiscoverable(ADV_IND, GAP_INITIATOR_ADDR_TYPE_PUBLIC, NULL,
        GAP_CHANNEL_MAP_ALL, GAP_FILTER_POLICY_SCAN_ANY_CONNECT_ANY);
    PT_WAIT_UNTIL(pt, hci_command_complete);
    // ignoring response data

    PT_END(pt);
}

pbio_error_t pbdrv_bluetooth_tx(uint8_t c) {
    // make sure we have a Bluetooth connection
    if (!uart_tx_notify_en) {
        return PBIO_ERROR_INVALID_OP;
    }

    // can't add the the buffer if tx is in progress or buffer is full
    if (uart_tx_busy || uart_tx_buf_size == NRF_CHAR_SIZE) {
        return PBIO_ERROR_AGAIN;
    }

    // append the char
    uart_tx_buf[uart_tx_buf_size] = c;
    uart_tx_buf_size++;

    // poke the process to start tx soon-ish. This way, we can accumulate up to
    // NRF_CHAR_SIZE bytes before actually transmitting
    // TODO: it probably would be better to poll here instead of using events
    // so that we don't fill up the event queue with 20 events
    process_post(&pbdrv_bluetooth_hci_process, PROCESS_EVENT_MSG, NULL);

    return PBIO_SUCCESS;
}

static PT_THREAD(uart_service_send_data(struct pt *pt))
{
    PT_BEGIN(pt);

retry:
    PT_WAIT_WHILE(pt, write_xfer_size);

    if (!uart_tx_notify_en) {
        PT_EXIT(pt);
    }

    {
        attHandleValueNoti_t req;

        req.handle = uart_tx_char_handle;
        req.len = uart_tx_buf_size;
        req.pValue = uart_tx_buf;
        ATT_HandleValueNoti(conn_handle, &req);
    }
    PT_WAIT_UNTIL(pt, hci_command_status);

    HCI_StatusCodes_t status = read_buf[8];
    if (status == blePending) {
        goto retry;
    }

    PT_END(pt);
}

PROCESS_THREAD(pbdrv_bluetooth_hci_process, ev, data) {
    static struct etimer timer;
    static struct pt child_pt;

    PROCESS_EXITHANDLER(bluetooth_deinit());

    PROCESS_BEGIN();

    bluetooth_init();

    while (true) {
        // make sure the Bluetooth chip is in reset long enough to actually reset
        etimer_set(&timer, clock_from_msec(150));
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        // take Bluetooth chip out of reset
        bluetooth_reset(RESET_STATE_OUT_HIGH);

        // not sure why we need this
        etimer_set(&timer, clock_from_msec(150));
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
        bluetooth_reset(RESET_STATE_INPUT);

        // bluetooth chip should not have any messages for us yet, so spi_srdy
        // should be false
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
                continue;
            }
        }

        etimer_set(&timer, clock_from_msec(100));
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        PROCESS_PT_SPAWN(&child_pt, hci_init(&child_pt));
        PROCESS_PT_SPAWN(&child_pt, init_pybricks_service(&child_pt));
        PROCESS_PT_SPAWN(&child_pt, init_uart_service(&child_pt));
        PROCESS_PT_SPAWN(&child_pt, set_discoverable(&child_pt));

        // TODO: we should have a timeout and stop scanning eventually
        PROCESS_WAIT_UNTIL(conn_handle != NO_CONNECTION);

        etimer_set(&timer, clock_from_msec(500));

        // conn_handle is set to 0 upon disconnection
        while (conn_handle != NO_CONNECTION) {
            PROCESS_WAIT_EVENT();
            if (ev == PROCESS_EVENT_TIMER && etimer_expired(&timer)) {
                etimer_reset(&timer);
                // just occasionally checking to see if we are still connected
                continue;
            }
            if (ev == PROCESS_EVENT_MSG && uart_tx_buf_size) {
                uart_tx_busy = true;
                PROCESS_PT_SPAWN(&child_pt, uart_service_send_data(&child_pt));
                uart_tx_busy = false;
                uart_tx_buf_size = 0;
            }
        }

        // reset Bluetooth chip
        bluetooth_reset(RESET_STATE_OUT_LOW);
    }

    PROCESS_END();
}
