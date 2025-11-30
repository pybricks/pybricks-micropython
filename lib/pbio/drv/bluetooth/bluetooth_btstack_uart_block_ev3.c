// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 UART driver for BlueKitchen BTStack (stubs).

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <btstack.h>
#include <btstack_uart_block.h>
#include <btstack_run_loop.h>
#include <pbdrv/gpio.h>
#include <pbdrv/uart.h>
#include <pbdrv/cache.h>
#include <pbio/os.h>

#include "bluetooth_btstack_uart_block_ev3.h"
#include "../gpio/gpio_ev3.h"
#include "../uart/uart_ev3.h"

#include <tiam1808/armv5/am1808/edma_event.h>
#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/edma.h>
#include <tiam1808/hw/hw_edma3cc.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_syscfg1_AM1808.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/psc.h>
#include <tiam1808/uart.h>

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
#else
#define DEBUG_PRINT(...)
#endif

void pbdrv_bluetooth_btstack_classic_run_loop_trigger(void);
void pbdrv_bluetooth_btstack_uart_block_ev3_handle_tx_complete(void);
void pbdrv_bluetooth_btstack_uart_block_ev3_handle_rx_complete(void);

// If a read has been requested, a pointer to the buffer and its length, else
// null and zero.
static uint8_t *read_buf;
static int read_buf_len;

// If a write has been requested, a pointer to the buffer and its length, else
// null and zero.
static const uint8_t *write_buf;
static int write_buf_len;

// Called when a block finishes sending.
static void (*block_sent)(void);
// Called when a block finishes being received.
static void (*block_received)(void);

static btstack_data_source_t pbdrv_bluetooth_btstack_classic_poll_data_source;
static volatile bool dma_write_complete;
static volatile bool dma_read_complete;

#define UART_PORT (SOC_UART_2_REGS)
#define DMA_CHA_TX (EDMA3_CHA_UART2_TX)
#define DMA_CHA_RX (EDMA3_CHA_UART2_RX)
#define EDMA_BASE (SOC_EDMA30CC_0_REGS)
#define DMA_EVENT_QUEUE (0)

void pbdrv_bluetooth_btstack_uart_block_ev3_handle_tx_complete(void) {
    EDMA3DisableTransfer(EDMA_BASE, DMA_CHA_TX, EDMA3_TRIG_MODE_EVENT);
    dma_write_complete = true;
    pbdrv_bluetooth_btstack_classic_run_loop_trigger();
}

void pbdrv_bluetooth_btstack_uart_block_ev3_handle_rx_complete(void) {
    EDMA3DisableTransfer(EDMA_BASE, DMA_CHA_RX, EDMA3_TRIG_MODE_EVENT);
    dma_read_complete = true;
    pbdrv_bluetooth_btstack_classic_run_loop_trigger();
}

static void pbdrv_bluetooth_btstack_classic_drive_read() {
    if (!read_buf || read_buf_len <= 0 || !dma_read_complete) {
        return;
    }

    pbdrv_cache_prepare_after_dma(read_buf, read_buf_len);
    read_buf = NULL;
    read_buf_len = 0;
    dma_read_complete = false;
    if (block_received) {
        block_received();
    }
}

static void pbdrv_bluetooth_btstack_classic_drive_write() {
    if (!write_buf || write_buf_len <= 0 || !dma_write_complete) {
        return;
    }

    write_buf = NULL;
    write_buf_len = 0;
    dma_write_complete = false;
    if (block_sent) {
        block_sent();
    }
}

static void pbdrv_bluetooth_btstack_classic_poll(struct btstack_data_source *ds, btstack_data_source_callback_type_t callback_type) {
    if (callback_type != DATA_SOURCE_CALLBACK_POLL) {
        return;
    }
    if (read_buf) {
        pbdrv_bluetooth_btstack_classic_drive_read();
    }
    if (write_buf) {
        pbdrv_bluetooth_btstack_classic_drive_write();
    }
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_init(const btstack_uart_config_t *config) {
    DEBUG_PRINT("[btc] Initializing UART block EV3\n");
    block_received = NULL;
    block_sent = NULL;

    // Before setting up the bluetooth module, turn on the UART.
    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_UART2, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    // Configure the UART pins.
    const pbdrv_gpio_t bluetooth_uart_rx = PBDRV_GPIO_EV3_PIN(4, 19, 16, 1, 3);
    const pbdrv_gpio_t bluetooth_uart_tx = PBDRV_GPIO_EV3_PIN(4, 23, 20, 1, 2);
    const pbdrv_gpio_t bluetooth_uart_rts = PBDRV_GPIO_EV3_PIN(0, 27, 24, 0, 9);
    const pbdrv_gpio_t bluetooth_uart_cts = PBDRV_GPIO_EV3_PIN(0, 31, 28, 0, 8);
    pbdrv_gpio_alt(&bluetooth_uart_rx, SYSCFG_PINMUX4_PINMUX4_19_16_UART2_RXD);
    pbdrv_gpio_alt(&bluetooth_uart_tx, SYSCFG_PINMUX4_PINMUX4_23_20_UART2_TXD);
    pbdrv_gpio_alt(&bluetooth_uart_rts, SYSCFG_PINMUX0_PINMUX0_27_24_UART2_RTS);
    pbdrv_gpio_alt(&bluetooth_uart_cts, SYSCFG_PINMUX0_PINMUX0_31_28_UART2_CTS);

    UARTEnable(UART_PORT);
    UARTConfigSetExpClk(UART_PORT, SOC_UART_2_MODULE_FREQ, config->baudrate, UART_WORDL_8BITS, UART_OVER_SAMP_RATE_16);

    return 0;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_open(void) {
    DEBUG_PRINT("Bluetooth: Opening UART block EV3\n");
    write_buf = NULL;
    write_buf_len = 0;
    read_buf = NULL;
    read_buf_len = 0;
    dma_write_complete = false;
    dma_read_complete = false;

    // Note: this also clears the FIFO in case it contained anything before.
    UARTFIFOEnable(UART_PORT);
    // Flow control is important for the CC256x: for one thing, the volume of
    // data is such that without it we could get into trouble. For another
    // thing, pulling CTS low is how the module signals that it's ready to talk.
    UARTModemControlSet(UART_PORT, UART_RTS | UART_AUTOFLOW);

    // Set up the UART DMA channels.
    UARTDMAEnable(UART_PORT, UART_DMAMODE | UART_FIFO_MODE | UART_RX_TRIG_LEVEL_1);
    EDMA3RequestChannel(EDMA_BASE, EDMA3_CHANNEL_TYPE_DMA, DMA_CHA_TX, DMA_CHA_TX, DMA_EVENT_QUEUE);
    EDMA3RequestChannel(EDMA_BASE, EDMA3_CHANNEL_TYPE_DMA, DMA_CHA_RX, DMA_CHA_RX, DMA_EVENT_QUEUE);

    btstack_run_loop_set_data_source_handler(&pbdrv_bluetooth_btstack_classic_poll_data_source, pbdrv_bluetooth_btstack_classic_poll);
    btstack_run_loop_enable_data_source_callbacks(&pbdrv_bluetooth_btstack_classic_poll_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&pbdrv_bluetooth_btstack_classic_poll_data_source);
    return 0;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_close(void) {
    btstack_run_loop_disable_data_source_callbacks(&pbdrv_bluetooth_btstack_classic_poll_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_remove_data_source(&pbdrv_bluetooth_btstack_classic_poll_data_source);

    UARTDMADisable(UART_PORT, (UART_RX_TRIG_LEVEL_1 | UART_FIFO_MODE));
    EDMA3FreeChannel(EDMA_BASE, EDMA3_CHANNEL_TYPE_DMA, DMA_CHA_TX, EDMA3_TRIG_MODE_EVENT, DMA_CHA_TX, DMA_EVENT_QUEUE);
    EDMA3FreeChannel(EDMA_BASE, EDMA3_CHANNEL_TYPE_DMA, DMA_CHA_RX, EDMA3_TRIG_MODE_EVENT, DMA_CHA_RX, DMA_EVENT_QUEUE);
    UARTDisable(UART_PORT);
    return 0;
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_set_block_received(void (*handler)(void)) {
    block_received = handler;
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_set_block_sent(void (*handler)(void)) {
    block_sent = handler;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_set_baudrate(uint32_t baud) {
    UARTConfigSetExpClk(UART_PORT, SOC_UART_2_MODULE_FREQ, baud, UART_WORDL_8BITS, UART_OVER_SAMP_RATE_16);
    return 0;
}

static int pbdrv_bluetooth_btstack_uart_block_ev3_set_parity(int parity) {
    // TODO: maybe implement the parity setting.
    return 0;
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_receive_block(uint8_t *buffer,
    uint16_t len) {
    if (!buffer || len == 0) {
        return;
    }

    read_buf = buffer;
    read_buf_len = len;
    dma_read_complete = false;

    // Configure DMA transfer parameters for RX
    volatile EDMA3CCPaRAMEntry paramSet = {
        // UART receive register address
        .srcAddr = UART_PORT + UART_RBR,
        .destAddr = (uint32_t)read_buf,
        // Number of bytes in an array
        .aCnt = 1,
        // Number of such arrays to be transferred
        .bCnt = len,
        // Number of frames of aCnt*bBcnt bytes to be transferred
        .cCnt = 1,
        // The src index should not increment since it is a h/w register
        .srcBIdx = 0,
        // The dst index should increment for every byte being transferred
        .destBIdx = 1,
        // Transfer mode
        .srcCIdx = 0,
        .destCIdx = 0,
        .linkAddr = 0xFFFF,
        .bCntReload = 0,
        .opt = EDMA3CC_OPT_SAM | ((DMA_CHA_RX << EDMA3CC_OPT_TCC_SHIFT) & EDMA3CC_OPT_TCC) | (1 << EDMA3CC_OPT_TCINTEN_SHIFT),
    };

    // Save configuration and start transfer
    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, DMA_CHA_RX, (EDMA3CCPaRAMEntry *)&paramSet);
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, DMA_CHA_RX, EDMA3_TRIG_MODE_EVENT);
}

static void pbdrv_bluetooth_btstack_uart_block_ev3_send_block(const uint8_t *data,
    uint16_t size) {
    if (!data || size == 0) {
        return;
    }

    write_buf = data;
    write_buf_len = size;
    dma_write_complete = false;

    // Configure DMA transfer parameters following uart_ev3.c pattern
    volatile EDMA3CCPaRAMEntry paramSet = {
        .srcAddr = (uint32_t)write_buf,
        // UART transmit register address
        .destAddr = UART_PORT + UART_THR,
        // Number of bytes in an array
        .aCnt = 1,
        // Number of such arrays to be transferred
        .bCnt = size,
        // Number of frames of aCnt*bBcnt bytes to be transferred
        .cCnt = 1,
        // The src index should increment for every byte being transferred
        .srcBIdx = 1,
        // The dst index should not increment since it is a h/w register
        .destBIdx = 0,
        // Transfer mode
        .srcCIdx = 0,
        .destCIdx = 0,
        .linkAddr = 0xFFFF,
        .bCntReload = 0,
        .opt = EDMA3CC_OPT_DAM | ((DMA_CHA_TX << EDMA3CC_OPT_TCC_SHIFT) & EDMA3CC_OPT_TCC) | (1 << EDMA3CC_OPT_TCINTEN_SHIFT),
    };

    pbdrv_cache_prepare_before_dma(write_buf, size);
    EDMA3SetPaRAM(SOC_EDMA30CC_0_REGS, DMA_CHA_TX, (EDMA3CCPaRAMEntry *)&paramSet);
    EDMA3EnableTransfer(SOC_EDMA30CC_0_REGS, DMA_CHA_TX, EDMA3_TRIG_MODE_EVENT);
}

static const btstack_uart_block_t pbdrv_bluetooth_btstack_uart_block_ev3_block_ev3 = {
    .init = pbdrv_bluetooth_btstack_uart_block_ev3_init,
    .open = pbdrv_bluetooth_btstack_uart_block_ev3_open,
    .close = pbdrv_bluetooth_btstack_uart_block_ev3_close,
    .set_block_received = pbdrv_bluetooth_btstack_uart_block_ev3_set_block_received,
    .set_block_sent = pbdrv_bluetooth_btstack_uart_block_ev3_set_block_sent,
    .set_baudrate = pbdrv_bluetooth_btstack_uart_block_ev3_set_baudrate,
    .set_parity = pbdrv_bluetooth_btstack_uart_block_ev3_set_parity,
    .set_flowcontrol = NULL,
    .receive_block = pbdrv_bluetooth_btstack_uart_block_ev3_receive_block,
    .send_block = pbdrv_bluetooth_btstack_uart_block_ev3_send_block,
    .get_supported_sleep_modes = NULL,
    .set_sleep = NULL,
    .set_wakeup_handler = NULL,
};

const btstack_uart_block_t *pbdrv_bluetooth_btstack_uart_block_ev3_instance(void) {
    return &pbdrv_bluetooth_btstack_uart_block_ev3_block_ev3;
}

#endif  // PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3
