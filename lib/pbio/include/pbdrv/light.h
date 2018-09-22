
#ifndef _PBDRV_LIGHT_H_
#define _PBDRV_LIGHT_H_

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/light.h>
#include <pbio/port.h>

/**
 * \addtogroup LightDriver Light I/O driver
 * @{
 */

/** @cond INTERNAL */

/**
 * Initializes the low level light blinking engine. This should be called only
 * once and must be called before using any other light functions.
 */
void _pbdrv_light_init(void);

/**
 * Releases the low level light driver. No light functions can be called after
 * calling this function.
 */
#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_light_deinit(void);
#else
static inline void _pbdrv_light_deinit(void) { }
#endif

/** @endcond */

/**
 * Sets the color of the light. The RGB values are "raw" values, meaning that
 * setting all values to 255 may not result in a white color because of
 * different maximum intensities of the component LEDs. Use
 * ::pbdrv_light_get_rgb_for_color() to get device-specific RGB values for
 * predefined colors.
 *
 * The light may not be capabile of displaying all colors or may not have
 * adjustable brightness. If a light is only white, the color values
 * will be averaged to give the final intensity. If the light only has one or
 * two of the possible three colors, the other color values will be ignored.
 * If the light is not capabile of adjusting the intensity, values < 128 will
 * be considered "off" and >= 128 will be considered as "on".
 * @param [in]  port    The light port
 * @param [in]  r       The red component
 * @param [in]  g       The green component
 * @param [in]  b       The blue component
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_light_set_rgb(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b);

/**
 * Gets the "raw" RGB values for a predefined color. These returned values
 * should be passed to ::pbdrv_light_set_rgb() to produce that color.
 * @param [in]  port        The light port
 * @param [in]  color       The color to look up
 * @param [out] r           The red component
 * @param [out] g           The green component
 * @param [out] b           The blue component
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_INVALID_ARG if the color value is not valid
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbdrv_light_get_rgb_for_color(pbio_port_t port, pbio_light_color_t color,
                                           uint8_t *r, uint8_t *g, uint8_t *b);

/** @}*/

#endif // _PBDRV_LIGHT_H_
