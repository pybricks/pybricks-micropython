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

    (void)timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // HACK: This waits until storage is completely done with SPI flash before we start
    PBIO_OS_AWAIT_UNTIL(state, pbsys_storage_settings_get_settings());

    // Once SPI flash init is finished, there is nothing further for us to do.
    // We are ready to start sampling.

    // TODO: Actually start sampling

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
}

void pbdrv_adc_ev3_shut_down_hack() {
    shut_down_hack = 1;
    pbio_os_request_poll();
}
int pbdrv_adc_ev3_is_shut_down_hack() {
    return shut_down_hack_done;
}

#endif // PBDRV_CONFIG_ADC_EV3
