// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC_NXT

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/adc.h>
#include <pbio/error.h>

void pbdrv_adc_init(void) {
}

pbio_error_t pbdrv_adc_await_new_samples(pbio_os_state_t *state, uint32_t *start_time_us, uint32_t future_us) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

// does a single conversion for the specified channel
pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {

    *value = 0;
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_ADC_NXT
