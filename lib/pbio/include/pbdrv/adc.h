/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * \addtogroup AnalogDriver Analog/digital converter I/O driver
 * @{
 */

#ifndef _PBDRV_ADC_H_
#define _PBDRV_ADC_H_

#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/error.h>

#if PBDRV_CONFIG_ADC

/** @cond INTERNAL */
void _pbdrv_adc_init(void);
void _pbdrv_adc_poll(uint32_t now);
#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_adc_deinit(void);
#else
static inline void _pbdrv_adc_deinit(void) { }
#endif
/** @endcond */

/**
 * Gets the raw analog value for the specified channel.
 * @param [in]  ch      The A/DC channel
 * @param [out] value   The raw value
 * @return              ::PBIO_SUCCESS on success ::PBIO_ERROR_INVALID_ARG if
 *                      the channel is not valid or ::PBIO_ERROR_IO if there
 *                      was an I/O error.
 */
pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value);

#else

static inline void _pbdrv_adc_init(void) { }
static inline void _pbdrv_adc_poll(uint32_t now) { }
static inline void _pbdrv_adc_deinit(void) { }
static inline pbio_error_t pbdrv_adc_get_ch(uint8_t ch, uint16_t *value) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif /* _PBDRV_ADC_H_ */

/** @} */
