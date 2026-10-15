// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_NXT

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/adc.h>
#include <pbdrv/clock.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include <at91sam7s256.h>
#include <nxos/nxt.h>

#include "adc_nxt.h"

#include "../rproc/rproc_nxt.h"

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

/**
 * Rate at which TC1 triggers a conversion of all enabled channels. A full
 * sweep takes about 20 µs, so this leaves the ADC idle most of the time.
 */
#define PBDRV_ADC_NXT_SWEEP_RATE_HZ (10000)

/**
 * AD1, AD2, AD3 and AD7 are pin 6 of sensor ports 1--4 respectively. AD0 and
 * AD5 have no known use, so they are left disconnected from their pins.
 */
#define PBDRV_ADC_NXT_CH_MASK ( \
    AT91C_ADC_CH1 | AT91C_ADC_CH2 | AT91C_ADC_CH3 | AT91C_ADC_CH7 | \
    (1 << PBDRV_ADC_NXT_CH_USB) | (1 << PBDRV_ADC_NXT_CH_BT_MODE))


#if DEBUG
static pbio_error_t pbdrv_adc_debug_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {
        for (int i = 0; i < 12; i++) {
            uint16_t value;
            if (pbdrv_adc_get_ch(i, &value) == PBIO_SUCCESS) {
                DEBUG_PRINT("ch %d: %d\t", i, value);
            }

        }
        DEBUG_PRINT("\n");
        PBIO_OS_AWAIT_MS(state, &timer, 1000);
    }

    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}
#endif // DEBUG

void pbdrv_adc_init(void) {

    *AT91C_PMC_PCER = (1 << AT91C_ID_TC1) | (1 << AT91C_ID_ADC);

    // The ADC has no timer of its own, so TC1 is used to produce one rising
    // edge on its (chip internal, not routed to a pin) TIOA output per sweep.
    *AT91C_TC1_CCR = AT91C_TC_CLKDIS;
    *AT91C_TC1_IDR = ~0;
    (void)*AT91C_TC1_SR;
    *AT91C_TC1_CMR = AT91C_TC_CLKS_TIMER_DIV1_CLOCK | AT91C_TC_WAVE |
        AT91C_TC_WAVESEL_UP_AUTO | AT91C_TC_ACPA_SET | AT91C_TC_ACPC_CLEAR |
        AT91C_TC_ASWTRG_SET;
    // TIMER_DIV1_CLOCK is MCK/2.
    *AT91C_TC1_RC = (NXT_CLOCK_FREQ / 2) / PBDRV_ADC_NXT_SWEEP_RATE_HZ;
    *AT91C_TC1_RA = *AT91C_TC1_RC / 2;
    *AT91C_TC1_CCR = AT91C_TC_CLKEN;
    *AT91C_TC1_CCR = AT91C_TC_SWTRG;

    *AT91C_ADC_CR = AT91C_ADC_SWRST;

    // 10-bit mode with PRESCAL 5 for a 4 MHz ADC clock (5 MHz is the maximum),
    // STARTUP 31 for 64 µs and SHTIM 3 for 750 ns sample and hold, giving
    // 3.25 µs per channel. Sleep mode is off, so startup is only paid once.
    *AT91C_ADC_MR = AT91C_ADC_TRGEN_EN | AT91C_ADC_TRGSEL_TIOA1 |
        (5 << 8) | (31 << 16) | (3 << 24);

    *AT91C_ADC_CHER = PBDRV_ADC_NXT_CH_MASK;

    #if DEBUG
    static pbio_os_process_t debug_process;
    pbio_os_process_start(&debug_process, pbdrv_adc_debug_process_thread, NULL);
    #endif
}

pbio_error_t pbdrv_adc_await_new_samples(pbio_os_state_t *state, uint32_t *start_time_us, uint8_t ch, uint32_t future_us) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (ch < PBDRV_ADC_NXT_NUM_CH_AT91) {
        // A sweep completes every 100 µs, so only the settling time of the
        // measured signal matters.
        *start_time_us = pbdrv_clock_get_us();
        PBIO_OS_AWAIT_UNTIL(state, pbio_util_time_has_passed(pbdrv_clock_get_us(), *start_time_us + future_us));
    } else {
        // The AVR samples at 333 Hz and then still has to ship the value over
        // the TWI link, which dwarfs any settling time.
        *start_time_us = pbdrv_clock_get_ms();
        PBIO_OS_AWAIT_UNTIL(state, pbio_util_time_has_passed(pbdrv_clock_get_ms(), *start_time_us + 7));
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {

    if (ch >= PBDRV_ADC_NXT_NUM_CH_AT91) {
        return pbdrv_rproc_nxt_get_sensor_adc(ch - PBDRV_ADC_NXT_NUM_CH_AT91, value);
    }

    if (!(PBDRV_ADC_NXT_CH_MASK & (1 << ch))) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Conversions are triggered by hardware, so the channel data register
    // always holds the newest sample without having to wait for anything.
    *value = AT91C_ADC_CDR0[ch];

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_ADC_NXT
