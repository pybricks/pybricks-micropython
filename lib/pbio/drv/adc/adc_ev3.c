// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_EV3

#if !PBDRV_CONFIG_BLOCK_DEVICE_EV3
#error "EV3 block device driver must be enabled"
#endif

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

#define PBDRV_ADC_EV3_NUM_DELAY_SAMPLES (2)

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

    ADC_SAMPLE_PERIOD = 2,
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

static const uint32_t channel_cmd[PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_ADC_EV3_NUM_DELAY_SAMPLES] = {
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
static volatile uint16_t channel_data[PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_ADC_EV3_NUM_DELAY_SAMPLES];

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
        a = channel_data[ch + PBDRV_ADC_EV3_NUM_DELAY_SAMPLES];
        b = channel_data[ch + PBDRV_ADC_EV3_NUM_DELAY_SAMPLES];
    } while (a != b);

    // Mask the data to 10 bits
    *value = (a >> 2) & 0x3ff;
    return PBIO_SUCCESS;
}

static pbio_os_process_t pbdrv_adc_ev3_process;

/**
 * Request ADC process to exit and await until it does.
 */
pbio_error_t pbdrv_adc_ev3_exit(pbio_os_state_t *state) {
    PBIO_OS_ASYNC_BEGIN(state);

    pbio_os_process_make_request(&pbdrv_adc_ev3_process, PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL);
    PBIO_OS_AWAIT_UNTIL(state, pbdrv_adc_ev3_process.err == PBIO_SUCCESS);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_error_t pbdrv_adc_ev3_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // Poll continuously until cancellation is requested.
    while (!(pbdrv_adc_ev3_process.request & PBIO_OS_PROCESS_REQUEST_TYPE_CANCEL)) {

        // Start a sample of all channels
        pbdrv_block_device_ev3_spi_begin_for_adc(
            channel_cmd,
            channel_data,
            PBDRV_CONFIG_ADC_EV3_ADC_NUM_CHANNELS + PBDRV_ADC_EV3_NUM_DELAY_SAMPLES);

        // Allow event loop to run once so that processes that await new
        // samples can begin awaiting completion of the transfer.
        PBIO_OS_AWAIT_ONCE_AND_POLL(state);

        // Await for actual transfer to complete.
        PBIO_OS_AWAIT_WHILE(state, pbdrv_block_device_ev3_is_busy());

        for (uint32_t i = 0; i < pbdrv_adc_callback_count; i++) {
            pbdrv_adc_callbacks[i]();
        }

        pbio_os_timer_set(&timer, ADC_SAMPLE_PERIOD);
        PBIO_OS_AWAIT_UNTIL(state, pbdrv_adc_ev3_process.request || pbio_os_timer_is_expired(&timer));
    }

    // Processes may be waiting on us to complete, so kick when done.
    pbio_os_request_poll();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_adc_await_new_samples(pbio_os_state_t *state) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT_UNTIL(state, pbdrv_block_device_ev3_is_busy());
    PBIO_OS_AWAIT_UNTIL(state, !pbdrv_block_device_ev3_is_busy());
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// Public init is not used.
void pbdrv_adc_init(void) {
}

// Init starts here instead. Called by SPI flash driver when it is done initializing.
void pbdrv_adc_ev3_init(void) {

    // Most of the SPI initialization is already done by the SPI flash driver.

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

    // Begin polling.
    pbio_os_process_start(&pbdrv_adc_ev3_process, pbdrv_adc_ev3_process_thread, NULL);
}

#endif // PBDRV_CONFIG_ADC_EV3
