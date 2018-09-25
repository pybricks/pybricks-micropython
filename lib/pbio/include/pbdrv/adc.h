/**
 * \addtogroup AnalogDriver Analog/digital converter I/O driver
 * @{
 */

#ifndef _PBDRV_ADC_H_
#define _PBDRV_ADC_H_

#include <stdint.h>

#include <pbio/error.h>

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

#endif /* _PBDRV_ADC_H_ */

/** @} */
