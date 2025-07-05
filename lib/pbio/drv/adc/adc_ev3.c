// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_EV3

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../core.h"

#include <pbdrv/adc.h>
#include <pbdrv/clock.h>
#include <pbdrv/gpio.h>

#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/util.h>

#include <tiam1808/spi.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#include "adc_ev3.h"
#include "../drv/block_device/block_device_ev3.h"
#include "../drv/gpio/gpio_ev3.h"

#include <pbdrv/../../drv/uart/uart_debug_first_port.h>

// PROCESS(pbdrv_adc_process, "ADC");

#define PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES (2)

/**
 * Bus speeds.
 */
enum {
    // The maximum ADC clock speed according to the datasheet is 20 MHz
    // which would require a division factor of /7.5 which is not possible
    // as this peripheral doesn't have a fractional clock generator.
    // The TI HAL does *not* perform rounding down and only truncates the divider,
    // which overclocks the chip. We _preemptively_ round this value down in order
    // to both prevent that as well as document what is actually happening.
    //
    // 150 MHz / 8 = 18.75 MHz
    SPI_CLK_SPEED_ADC = 18750000,
};

// Construct both SPI peripheral settings (data format, chip select)
// and ADC chip settings (manual mode, 2xVref) in one go,
// so that DMA can be used efficiently
#define MANUAL_ADC_CHANNEL(x)                                                       \
    (SPI_SPIDAT1_DFSEL_FORMAT1 << SPI_SPIDAT1_DFSEL_SHIFT) |                        \
    (0 << (SPI_SPIDAT1_CSNR_SHIFT + 3)) | (1 << (SPI_SPIDAT1_CSNR_SHIFT + 0)) |     \
    (1 << 12) |                                                                     \
    (1 << 11) |                                                                     \
    (((x) & 0xf) << 7) |                                                            \
    (1 << 6)

static const uint32_t channel_cmd[PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES] = {
    MANUAL_ADC_CHANNEL(0),
    MANUAL_ADC_CHANNEL(1),
    MANUAL_ADC_CHANNEL(2),
    MANUAL_ADC_CHANNEL(3),
    MANUAL_ADC_CHANNEL(4),
    MANUAL_ADC_CHANNEL(5),
    MANUAL_ADC_CHANNEL(6),
    MANUAL_ADC_CHANNEL(7),
    MANUAL_ADC_CHANNEL(8),
    MANUAL_ADC_CHANNEL(9),
    MANUAL_ADC_CHANNEL(10),
    MANUAL_ADC_CHANNEL(11),
    MANUAL_ADC_CHANNEL(12),
    MANUAL_ADC_CHANNEL(13),
    MANUAL_ADC_CHANNEL(14),
    MANUAL_ADC_CHANNEL(15),

    MANUAL_ADC_CHANNEL(15),
    MANUAL_ADC_CHANNEL(15),
};
// XXX What are the atomicity guarantees around this? What guarantees do we need?
static volatile uint16_t channel_data[PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES];
static volatile uint8_t channel_data_index = 0;
static volatile bool adc_busy = false;

static pbdrv_adc_callback_t pbdrv_adc_callbacks[1];
static uint32_t pbdrv_adc_callback_count = 0;

void pbdrv_adc_set_callback(pbdrv_adc_callback_t callback) {
    if (pbdrv_adc_callback_count < PBIO_ARRAY_SIZE(pbdrv_adc_callbacks)) {
        pbdrv_adc_callbacks[pbdrv_adc_callback_count++] = callback;
    }
}

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    *value = 1234;
    return PBIO_SUCCESS;
    // if (ch >= PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS) {
    //     return PBIO_ERROR_INVALID_ARG;
    // }
    // // Values for the requested channel are received several samples later.
    // // The data only appears 12-bit but the last 2 bits are always zero.
    // *value = (channel_data[ch + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES] - 4096 * ch) >> 2;
    // return PBIO_SUCCESS;
}

// static void spi0_isr(void) {
//     uint32_t intCode = 0;
//     IntSystemStatusClear(SYS_INT_SPINT0);

//     while ((intCode = SPIInterruptVectorGet(SOC_SPI_0_REGS))) {
//         if (intCode != SPI_TX_BUF_EMPTY) {
//             continue;
//         }
//         // Payload encoding comes from the original EV3 sources, but we
//         // use the hardware SPI peripheral instead of bit-banging.
//         uint16_t payload = 0x1840 | (((channel_data_index % PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS) & 0x000F) << 7);
//         HWREG(SOC_SPI_0_REGS + SPI_SPIDAT0) = payload;
//         channel_data[channel_data_index] = SPIDataReceive(SOC_SPI_0_REGS);

//         if (++channel_data_index == PBIO_ARRAY_SIZE(channel_data)) {
//             SPIIntDisable(SOC_SPI_0_REGS, SPI_TRANSMIT_INT);
//             adc_busy = false;
//             process_poll(&pbdrv_adc_process);
//         }
//     }
// }

// static void pbdrv_adc_exit(void) {
//     SPIIntDisable(SOC_SPI_0_REGS, SPI_RECV_INT | SPI_TRANSMIT_INT);
// }

// // ADC / Flash SPI0 data MOSI
// static const pbdrv_gpio_t pin_spi0_mosi = PBDRV_GPIO_EV3_PIN(3, 15, 12, 8, 5);

// // ADC / Flash SPI0 data MISO
// static const pbdrv_gpio_t pin_spi0_miso = PBDRV_GPIO_EV3_PIN(3, 11, 8, 8, 6);

// // LCD SPI0 Clock
// static const pbdrv_gpio_t pin_spi0_clk = PBDRV_GPIO_EV3_PIN(3, 3, 0, 1, 8);

// // ADC / Flash SPI0 chip select (active low).
// static const pbdrv_gpio_t pin_spi0_cs = PBDRV_GPIO_EV3_PIN(3, 27, 24, 8, 2);

// // ADCACK PIN
// static const pbdrv_gpio_t pin_adc_ack = PBDRV_GPIO_EV3_PIN(19, 19, 16, 6, 2);

// // ADCBATEN
// static const pbdrv_gpio_t pin_adc_bat_en = PBDRV_GPIO_EV3_PIN(1, 7, 4, 0, 6);

static pbio_os_process_t pbdrv_adc_ev3_init_process;

pbio_error_t pbdrv_adc_ev3_init_process_thread(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_UNTIL(state, pbdrv_block_device_ev3_init_is_done());

    pbdrv_uart_debug_printf("adc init init init\r\n");

    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());
    for (int i = 0; i < 18; i++) {
        pbdrv_uart_debug_printf("%04x\r\n", channel_data[i] & 0xffff);
    }


    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());
    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());
    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());
    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());
    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());
    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());


    pbdrv_uart_debug_printf("adc 2222\r\n");

    pbdrv_block_device_ev3_spi_begin_for_adc(channel_cmd, channel_data, PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
    PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());

    for (int i = 0; i < 18; i++) {
        pbdrv_uart_debug_printf("%04x\r\n", channel_data[i] & 0xffff);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_adc_init(void) {
    // Immediately go into async mode so that we can wait for the SPI flash driver
    pbdrv_init_busy_up();
    pbio_os_process_start(&pbdrv_adc_ev3_init_process, pbdrv_adc_ev3_init_process_thread, NULL);

    // process_start(&pbdrv_adc_process);
}

void pbdrv_adc_update_soon(void) {
    // process_poll(&pbdrv_adc_process);
}

// PROCESS_THREAD(pbdrv_adc_process, ev, data) {
//     PROCESS_EXITHANDLER(pbdrv_adc_exit());

//     static struct etimer etimer;

//     PROCESS_BEGIN();

//     etimer_set(&etimer, 10);
//     for (;;) {
//         PROCESS_WAIT_EVENT_UNTIL((ev == PROCESS_EVENT_TIMER && etimer_expired(&etimer)) || ev == PROCESS_EVENT_POLL);

//         channel_data_index = 0;
//         adc_busy = true;
//         SPIEnable(SOC_SPI_0_REGS);
//         SPIIntEnable(SOC_SPI_0_REGS, SPI_TRANSMIT_INT);
//         PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL && !adc_busy);

//         for (uint32_t i = 0; i < pbdrv_adc_callback_count; i++) {
//             pbdrv_adc_callbacks[i]();
//         }

//         etimer_reset(&etimer);
//     }

//     PROCESS_END();
// }

void pbdrv_adc_ev3_configure_data_format() {
    SPIClkConfigure(SOC_SPI_0_REGS, SOC_SYSCLK_2_FREQ, SPI_CLK_SPEED_ADC, SPI_DATA_FORMAT1);
    // NOTE: Cannot be CPOL=1 CPHA=1 like SPI flash
    // The ADC seems to use the last falling edge to trigger conversions (see Figure 1 in the datasheet).
    SPIConfigClkFormat(SOC_SPI_0_REGS, SPI_CLK_POL_LOW | SPI_CLK_OUTOFPHASE, SPI_DATA_FORMAT1);
    SPIShiftMsbFirst(SOC_SPI_0_REGS, SPI_DATA_FORMAT1);
    SPICharLengthSet(SOC_SPI_0_REGS, 16, SPI_DATA_FORMAT1);
}

#endif // PBDRV_CONFIG_ADC_EV3
