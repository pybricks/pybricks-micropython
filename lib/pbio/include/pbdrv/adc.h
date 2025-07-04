// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

/**
 * @addtogroup AnalogDriver Driver: Analog/digital converter (ADC)
 * @{
 */

#ifndef _PBDRV_ADC_H_
#define _PBDRV_ADC_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

typedef void (*pbdrv_adc_callback_t)(void);

#if PBDRV_CONFIG_ADC

/**
 * Gets the raw analog value for the specified channel.
 * @param [in]  ch      The A/DC channel
 * @param [out] value   The raw value
 * @return              ::PBIO_SUCCESS on success ::PBIO_ERROR_INVALID_ARG if
 *                      the channel is not valid or ::PBIO_ERROR_IO if there
 *                      was an I/O error.
 */
pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value);

/**
 * Sets a callback to be called when the ADC has new data.
 *
 * NB: Not implemented on all platforms.
 *
 * @param [in] callback The callback function.
 */
void pbdrv_adc_set_callback(pbdrv_adc_callback_t callback);

/**
 * Requests the ADC to update soon.
 *
 * NB: Not implemented on all platforms.
 */
void pbdrv_adc_update_soon(void);

#else

static inline pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    *value = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_adc_set_callback(pbdrv_adc_callback_t callback) {
}

static inline void pbdrv_adc_update_soon(void) {
}

#endif

#endif /* _PBDRV_ADC_H_ */

/** @} */
