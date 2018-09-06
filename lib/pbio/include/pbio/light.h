
#ifndef _PBIO_LIGHT_H_
#define _PBIO_LIGHT_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

typedef enum {
    PBIO_LIGHT_PATTERN_OFF,     /**< The light is off */
    PBIO_LIGHT_PATTERN_ON,      /**< The light is on (solid) */
} pbio_light_pattern_t;

/**
 * Initializes the low level light blinking engine. This should be called only
 * once and must be called before using any other light functions.
 */
void pbio_light_init(void);

/**
 * Releases the low level light driver. No light functions can be called after
 * calling this function.
 */
void pbio_light_deinit(void);

/**
 * Sets the color of the light. The light may not be capabile of displaying
 * all colors or varying intensities. If a light is only white, the color values
 * will be averaged to give the final intensity. If the light only has one or
 * two of the possible three colors, the other color values will be ignored.
 * If the light is not capabile of adjusting the intensity, values < 128 will
 * be considered "off" and >= 128 will be considered as "on".
 * @param [in]  port    The light port
 * @param [in]  r       The red component
 * @param [in]  g       The green component
 * @param [in]  b       The blue component
 * @return              ::PB_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_light_set_color(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b);

/**
 * Sets the blink pattern of the light.
 * @param [in]  port    The light port
 * @param [in]  pattern The pattern
 * @return              ::PB_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                      ::PBIO_ERROR_INVALID_ARG if the pattern is not valid
 *                      ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbio_light_set_pattern(pbio_port_t port, pbio_light_pattern_t pattern);

#endif // _PBIO_LIGHT_H_
