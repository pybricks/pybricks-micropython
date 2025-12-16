// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_NXT

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/adc.h>
#include <pbdrv/clock.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include "../rproc/rproc_nxt.h"

void pbdrv_adc_init(void) {
}

pbio_error_t pbdrv_adc_await_new_samples(pbio_os_state_t *state, uint32_t *start_time_us, uint32_t future_us) {

    PBIO_OS_ASYNC_BEGIN(state);
    *start_time_us = pbdrv_clock_get_ms();
    // REVISIT: Pass channel ID to this function so we can adjust the wait time
    // to the specific sample coming in. The AVR adc samples will be slower.
    PBIO_OS_AWAIT_UNTIL(state, pbio_util_time_has_passed(pbdrv_clock_get_ms(), *start_time_us + 7));
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {

    if (ch < 4) {
        return pbdrv_rproc_nxt_get_sensor_adc(ch, value);
    }

    // TODO: AT91SAM7S256 internal ADC.
    *value = 0;
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_ADC_NXT
