// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

// Random number generator using analog/digital convert as source of entropy.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RANDOM_ADC

#include <stdint.h>

#include <pbio/error.h>

static uint32_t pbdrv_random_adc_value;

/**
 * Pushes the LSB of a value obtained from an analog source to the random
 * number source.
 * @param [in]  value   A raw analog value.
 */
void pbdrv_random_adc_push_lsb(uint16_t value) {
    pbdrv_random_adc_value = (pbdrv_random_adc_value << 1) | (value & 1);
}

// Internal API implementation

void pbdrv_random_init(void) {
}

// Public API implementation

pbio_error_t pbdrv_random_get(uint32_t *random) {
    // TODO: keep track of reads and writes to pbdrv_adc_random and return
    // PBIO_ERROR_AGAIN when appropriate.
    *random = pbdrv_random_adc_value;

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_RANDOM_ADC
