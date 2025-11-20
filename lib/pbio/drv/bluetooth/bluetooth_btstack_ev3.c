// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include <btstack.h>
#include <btstack_run_loop.h>
#include <btstack_uart_block.h>
#include "btstack_chipset_cc256x.h"
#include "hci_transport_h4.h"


#include "bluetooth_btstack.h"
#include "bluetooth_btstack_ev3.h"

#include <pbdrv/cache.h>
#include <pbdrv/gpio.h>
#include <pbdrv/uart.h>

#include <pbio/error.h>
#include <pbio/os.h>

#include "../gpio/gpio_ev3.h"
#include "../uart/uart_ev3.h"

#include <tiam1808/armv5/am1808/edma_event.h>
#include <tiam1808/armv5/am1808/interrupt.h>
#include <tiam1808/ecap.h>
#include <tiam1808/edma.h>
#include <tiam1808/hw/hw_edma3cc.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_syscfg1_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/psc.h>
#include <tiam1808/uart.h>

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
static void pbdrv_hci_dump_reset(void) {
}
static void pbdrv_hci_dump_log_packet(uint8_t packet_type, uint8_t in, uint8_t *packet, uint16_t len) {
    pbdrv_uart_debug_printf("HCI %s packet type: %02x, len: %u\n", in ? "in" : "out", packet_type, len);
}
static void pbdrv_hci_dump_log_message(int log_level, const char *format, va_list argptr) {
    pbdrv_uart_debug_vprintf(format, argptr);
    pbdrv_uart_debug_printf("\n");
}
static const hci_dump_t bluetooth_btstack_classic_hci_dump = {
    .reset = pbdrv_hci_dump_reset,
    .log_packet = pbdrv_hci_dump_log_packet,
    .log_message = pbdrv_hci_dump_log_message,
};
#else
#define DEBUG_PRINT(...)
#endif

void pbdrv_bluetooth_btstack_set_chipset(uint16_t lmp_pal_subversion) {
    const pbdrv_bluetooth_btstack_chipset_info_t *info = lmp_pal_subversion == cc2560_info.lmp_version ?
        &cc2560_info : &cc2560a_info;
    btstack_chipset_cc256x_set_init_script((uint8_t *)info->init_script, info->init_script_size);

    // Needed to apply init script.
    const pbdrv_bluetooth_btstack_platform_data_t *pdata =
        &pbdrv_bluetooth_btstack_platform_data;
    hci_set_chipset(pdata->chipset_instance());
};

static const pbdrv_gpio_t pin_bluetooth_enable = PBDRV_GPIO_EV3_PIN(9, 27, 24, 4, 9);

static int ev3_control_off() {
    pbdrv_gpio_out_low(&pin_bluetooth_enable);
    return 0;
}

static void ev3_control_init(const void *config) {
    // From the ev3dev configuration:
    //
    // There is a PIC microcontroller for interfacing with an Apple MFi
    // chip. This interferes with normal Bluetooth operation, so we need
    // to make sure it is turned off. Note: The publicly available
    // schematics from LEGO don't show that these pins are connected to
    // anything, but they are present in the source code from LEGO.
    const pbdrv_gpio_t bt_pic_en = PBDRV_GPIO_EV3_PIN(8, 19, 16, 3, 3);
    pbdrv_gpio_alt(&bt_pic_en, SYSCFG_PINMUX8_PINMUX8_19_16_GPIO3_3);
    pbdrv_gpio_out_low(&bt_pic_en);
    // Hold RTS high (we're not ready to receive anything from the PIC).
    const pbdrv_gpio_t bt_pic_rts = PBDRV_GPIO_EV3_PIN(9, 7, 4, 4, 14);
    pbdrv_gpio_alt(&bt_pic_rts, SYSCFG_PINMUX9_PINMUX9_7_4_GPIO4_14);
    pbdrv_gpio_out_high(&bt_pic_rts);
    // CTS technically does not need to be configured, but for documentation
    // purposes we do.
    const pbdrv_gpio_t bt_pic_cts = PBDRV_GPIO_EV3_PIN(12, 3, 0, 5, 7);
    pbdrv_gpio_alt(&bt_pic_cts, SYSCFG_PINMUX12_PINMUX12_3_0_GPIO5_7);
    pbdrv_gpio_input(&bt_pic_cts);
    // Don't interfere with the BT clock's enable pin.
    const pbdrv_gpio_t bt_clock_en = PBDRV_GPIO_EV3_PIN(1, 11, 8, 0, 5);
    pbdrv_gpio_alt(&bt_clock_en, SYSCFG_PINMUX1_PINMUX1_11_8_GPIO0_5);
    pbdrv_gpio_input(&bt_clock_en);

    // Configure ECAP2 to emit the slow clock signal for the bluetooth module.
    ECAPOperatingModeSelect(SOC_ECAP_2_REGS, ECAP_APWM_MODE);
    // Calculate the number of clock ticks the APWM period should last. Note
    // that the following float operations are all constant and optimized away.
    // APWM is clocked by sysclk2 by default.
    // Target frequency is 32.767 kHz, see cc2560 datasheet.
    // Note that the APWM module wraps on the cycle after reaching the period
    // value, which means we need to subtract one from the desired period to get
    // a period length in cycles that matches the desired frequency.
    const int aprd = round(SOC_SYSCLK_2_FREQ / 32767.0) - 1;
    ECAPAPWMCaptureConfig(SOC_ECAP_2_REGS, aprd / 2, aprd);
    // Set the polarity to active high. It doesn't matter which it is but for
    // the sake of determinism we set it explicitly.
    ECAPAPWMPolarityConfig(SOC_ECAP_2_REGS, ECAP_APWM_ACTIVE_HIGH);
    // Disable input and output synchronization.
    ECAPSyncInOutSelect(SOC_ECAP_2_REGS, ECAP_SYNC_IN_DISABLE, ECAP_SYNC_OUT_DISABLE);
    // Start the counter running.
    ECAPCounterControl(SOC_ECAP_2_REGS, ECAP_COUNTER_FREE_RUNNING);
    // Set gp0[7] to output the ECAP2 APWM signal.
    const pbdrv_gpio_t bluetooth_slow_clock = PBDRV_GPIO_EV3_PIN(1, 3, 0, 0, 7);
    pbdrv_gpio_alt(&bluetooth_slow_clock, SYSCFG_PINMUX1_PINMUX1_3_0_ECAP2);

    pbdrv_gpio_alt(&pin_bluetooth_enable, SYSCFG_PINMUX9_PINMUX9_27_24_GPIO4_9);

    // Start the module in a defined (and disabled) state.
    ev3_control_off();

    #if DEBUG
    hci_dump_init(&bluetooth_btstack_classic_hci_dump);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_INFO, true);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_ERROR, true);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_DEBUG, true);
    #endif

    DEBUG_PRINT("Bluetooth: Finished init control\n");
}

static int ev3_control_on() {
    // Note: the module is not actually "on" yet, however, the way it signals
    // its on-ness is by unblocking our ability to send UART messages. We
    // use auto flow control on the UART, so we don't actually need to wait
    // for the module to come up here.
    pbdrv_gpio_out_high(&pin_bluetooth_enable);
    return 0;
}

static const btstack_control_t ev3_control = {
    .init = &ev3_control_init,
    .on = &ev3_control_on,
    .off = &ev3_control_off,
    .sleep = NULL,
    .wake = NULL,
    .register_for_power_notifications = NULL,
};

const btstack_control_t *pbdrv_bluetooth_btstack_ev3_control_instance(void) {
    return &ev3_control;
}


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

void pbdrv_bluetooth_btstack_ev3_handle_tx_complete(void) {
    EDMA3DisableTransfer(EDMA_BASE, DMA_CHA_TX, EDMA3_TRIG_MODE_EVENT);
    dma_write_complete = true;
    pbdrv_bluetooth_btstack_run_loop_trigger();
}

void pbdrv_bluetooth_btstack_ev3_handle_rx_complete(void) {
    EDMA3DisableTransfer(EDMA_BASE, DMA_CHA_RX, EDMA3_TRIG_MODE_EVENT);
    dma_read_complete = true;
    pbdrv_bluetooth_btstack_run_loop_trigger();
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

static int pbdrv_bluetooth_btstack_ev3_init(const btstack_uart_config_t *config) {
    DEBUG_PRINT("Bluetooth: Initializing UART block EV3.\n");
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

static int pbdrv_bluetooth_btstack_ev3_open(void) {
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

static int pbdrv_bluetooth_btstack_ev3_close(void) {
    btstack_run_loop_disable_data_source_callbacks(&pbdrv_bluetooth_btstack_classic_poll_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_remove_data_source(&pbdrv_bluetooth_btstack_classic_poll_data_source);

    UARTDMADisable(UART_PORT, (UART_RX_TRIG_LEVEL_1 | UART_FIFO_MODE));
    EDMA3FreeChannel(EDMA_BASE, EDMA3_CHANNEL_TYPE_DMA, DMA_CHA_TX, EDMA3_TRIG_MODE_EVENT, DMA_CHA_TX, DMA_EVENT_QUEUE);
    EDMA3FreeChannel(EDMA_BASE, EDMA3_CHANNEL_TYPE_DMA, DMA_CHA_RX, EDMA3_TRIG_MODE_EVENT, DMA_CHA_RX, DMA_EVENT_QUEUE);
    UARTDisable(UART_PORT);
    return 0;
}

static void pbdrv_bluetooth_btstack_ev3_set_block_received(void (*handler)(void)) {
    block_received = handler;
}

static void pbdrv_bluetooth_btstack_ev3_set_block_sent(void (*handler)(void)) {
    block_sent = handler;
}

static int pbdrv_bluetooth_btstack_ev3_set_baudrate(uint32_t baud) {
    UARTConfigSetExpClk(UART_PORT, SOC_UART_2_MODULE_FREQ, baud, UART_WORDL_8BITS, UART_OVER_SAMP_RATE_16);
    return 0;
}

static int pbdrv_bluetooth_btstack_ev3_set_parity(int parity) {
    // TODO: maybe implement the parity setting.
    return 0;
}

static void pbdrv_bluetooth_btstack_ev3_receive_block(uint8_t *buffer,
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

static void pbdrv_bluetooth_btstack_ev3_send_block(const uint8_t *data,
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

static const btstack_uart_block_t pbdrv_bluetooth_btstack_ev3_block_ev3 = {
    .init = pbdrv_bluetooth_btstack_ev3_init,
    .open = pbdrv_bluetooth_btstack_ev3_open,
    .close = pbdrv_bluetooth_btstack_ev3_close,
    .set_block_received = pbdrv_bluetooth_btstack_ev3_set_block_received,
    .set_block_sent = pbdrv_bluetooth_btstack_ev3_set_block_sent,
    .set_baudrate = pbdrv_bluetooth_btstack_ev3_set_baudrate,
    .set_parity = pbdrv_bluetooth_btstack_ev3_set_parity,
    .set_flowcontrol = NULL,
    .receive_block = pbdrv_bluetooth_btstack_ev3_receive_block,
    .send_block = pbdrv_bluetooth_btstack_ev3_send_block,
    .get_supported_sleep_modes = NULL,
    .set_sleep = NULL,
    .set_wakeup_handler = NULL,
};

const hci_transport_t *pbdrv_bluetooth_btstack_ev3_transport_instance(void) {
    return hci_transport_h4_instance_for_uart(&pbdrv_bluetooth_btstack_ev3_block_ev3);
}

const void *pbdrv_bluetooth_btstack_ev3_transport_config(void) {
    static const hci_transport_config_uart_t config = {
        .type = HCI_TRANSPORT_CONFIG_UART,
        .baudrate_init = 115200,
        // Note: theoretically the AM1808 should be able to go up to 1875000 or
        // higher, but we observed random lost transfers at that speed. 921600 seems
        // stable and is still plenty of bandwidth for Bluetooth classic.
        .baudrate_main = 921600,
        .flowcontrol = 1,
        .device_name = NULL,
    };
    return &config;
}

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3
