// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_EV3

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

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

#include "../sys/storage.h"

#define PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES (2)

/**
 * Constants.
 */
enum {
    // The maximum ADC clock speed according to the datasheet is 20 MHz.
    // However, because the SPI peripheral does not have a fractional clock generator,
    // the closest achievable in-spec speed is a division factor of 8.
    //
    // 150 MHz / 8 = 18.75 MHz actual
    SPI_CLK_SPEED_ADC = 20000000,

    ADC_SAMPLE_PERIOD = 10,
};

// Construct both SPI peripheral settings (data format, chip select)
// and ADC chip settings (manual mode, 2xVref) in one go,
// so that DMA can be used efficiently.
//
// NOTE: CSHOLD is *not* set here, so that CS is deasserted between each 16-bit unit
#define MANUAL_ADC_CHANNEL(x)                                                       \
    (1 << 26) |                                                                     \
    (SPI_SPIDAT1_DFSEL_FORMAT1 << SPI_SPIDAT1_DFSEL_SHIFT) |                        \
    (0 << (SPI_SPIDAT1_CSNR_SHIFT + PBDRV_EV3_SPI0_ADC_CS)) |                       \
    (1 << (SPI_SPIDAT1_CSNR_SHIFT + PBDRV_EV3_SPI0_FLASH_CS)) |                     \
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
    // We need two additional commands here because of how the ADC works.
    // In every given command frame, a new analog channel is selected in the ADC frontend multiplexer.
    // In frame n+1, that value actually gets converted to a digital value.
    // In frame n+2, the converted digital value is finally output, and we are able to receive it.
    // These requests are _pipelined_, so there is a latency of 2 frames, but we get a new sample on each frame.
    //
    // For more information, see figures 1 and 51 in the ADS7957 datasheet.
    MANUAL_ADC_CHANNEL(15),
    MANUAL_ADC_CHANNEL(15),
};
static volatile uint16_t channel_data[PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES];

static int adc_soon;
// Used to block ADC from interfering with flash upon shutdown
static int shut_down_hack = 0;
static int shut_down_hack_done = 0;

static pbdrv_adc_callback_t pbdrv_adc_callbacks[1];
static uint32_t pbdrv_adc_callback_count = 0;

void pbdrv_adc_set_callback(pbdrv_adc_callback_t callback) {
    if (pbdrv_adc_callback_count < PBIO_ARRAY_SIZE(pbdrv_adc_callbacks)) {
        pbdrv_adc_callbacks[pbdrv_adc_callback_count++] = callback;
    }
}

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    if (ch >= PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // XXX We probably need to figure out how atomicity works between the DMA and the CPU.
    // For now, read the value twice and assume it's good (not torn) if the values are the same.
    uint16_t a, b;
    do {
        // Values for the requested channel are received several samples later.
        a = channel_data[ch + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES];
        b = channel_data[ch + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES];
    } while (a != b);

    // Mask the data to 10 bits
    *value = (a >> 2) & 0x3ff;
    return PBIO_SUCCESS;
}

static pbio_os_process_t pbdrv_adc_ev3_process;

pbio_error_t pbdrv_adc_ev3_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // HACK: This waits until storage is completely done with SPI flash before we start
    PBIO_OS_AWAIT_UNTIL(state, pbsys_storage_settings_get_settings());

    // Once SPI flash init is finished, there is nothing further for us to do.
    // We are ready to start sampling.

    pbio_os_timer_set(&timer, ADC_SAMPLE_PERIOD);

    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, shut_down_hack || adc_soon || pbio_os_timer_is_expired(&timer));

        if (shut_down_hack) {
            shut_down_hack_done = 1;
            break;
        }

        if (adc_soon) {
            adc_soon = 0;
            pbio_os_timer_set(&timer, ADC_SAMPLE_PERIOD);
        } else {
            // TODO: There should probably be a pbio OS function for this
            timer.start += timer.duration;
        }

        // Do a sample of all channels
        pbdrv_block_device_ev3_spi_begin_for_adc(
            channel_cmd,
            channel_data,
            PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_CONFIG_ADC_EV3_NUM_DELAY_SAMPLES);
        PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());

        for (uint32_t i = 0; i < pbdrv_adc_callback_count; i++) {
            pbdrv_adc_callbacks[i]();
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_adc_init(void) {
    // Immediately go into async mode so that we can wait for the SPI flash driver.
    // We *don't* want to block the initial init phase, or else things will deadlock.

    pbio_os_process_start(&pbdrv_adc_ev3_process, pbdrv_adc_ev3_process_thread, NULL);
}

void pbdrv_adc_update_soon(void) {
    adc_soon = 1;
    pbio_os_request_poll();
}

void pbdrv_adc_ev3_configure_data_format() {
    SPIClkConfigure(SOC_SPI_0_REGS, SOC_SYSCLK_2_FREQ, SPI_CLK_SPEED_ADC, SPI_DATA_FORMAT1);
    // NOTE: Cannot be CPOL=1 CPHA=1 like SPI flash
    // The ADC seems to use the last falling edge to trigger conversions (see Figure 1 in the datasheet).
    SPIConfigClkFormat(SOC_SPI_0_REGS, SPI_CLK_POL_LOW | SPI_CLK_OUTOFPHASE, SPI_DATA_FORMAT1);
    SPIShiftMsbFirst(SOC_SPI_0_REGS, SPI_DATA_FORMAT1);
    SPICharLengthSet(SOC_SPI_0_REGS, 16, SPI_DATA_FORMAT1);
    // In order to compensate for analog impedance issues and capacitor charging time,
    // we set all SPI delays to the maximum for the ADC. This helps get more accurate readings.
    // This includes both this delay (the delay where CS is held inactive),
    // as well as the CS-assert-to-clock-start and clock-end-to-CS-deassert delays
    // (which are global and set in block_device_ev3.c).
    SPIWdelaySet(SOC_SPI_0_REGS, 0x3f << SPI_SPIFMT_WDELAY_SHIFT, SPI_DATA_FORMAT1);
}

void pbdrv_adc_ev3_shut_down_hack() {
    shut_down_hack = 1;
    pbio_os_request_poll();
}
int pbdrv_adc_ev3_is_shut_down_hack() {
    return shut_down_hack_done;
}

#endif // PBDRV_CONFIG_ADC_EV3
