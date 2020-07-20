// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * \addtogroup Light Light control functions
 * @{
 */

#ifndef _PBIO_LIGHT_H_
#define _PBIO_LIGHT_H_

#include <stdbool.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/port.h>

/**
 * Light patterns.
 */
typedef enum {
    PBIO_LIGHT_PATTERN_NONE,    /**< The light does not change */
    PBIO_LIGHT_PATTERN_FLASH,   /**< The light flashes */
    PBIO_LIGHT_PATTERN_BREATHE, /**< The light breathes */
} pbio_light_pattern_t;

#if PBDRV_CONFIG_LIGHT

/**
 * Turns the light on. Some lights may not be capable of display all colors or
 * any colors at all. Some lights may not have adjustable brightness.
 * @param [in]  port        The light port
 * @param [in]  color       The color
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_INVALID_ARG if the color value is not valid
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbio_light_on(pbio_port_t port, pbio_color_t color);

/**
 * Turns the light on. Some lights may not be capable of display all colors or
 * any colors at all. Some lights may not have adjustable brightness.
 * @param [in]  port        The light port
 * @param [in]  color       The color
 * @param [in]  pattern     The pattern
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_INVALID_ARG if the color value or pattern value is not valid
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbio_light_on_with_pattern(pbio_port_t port, pbio_color_t color, pbio_light_pattern_t pattern);

/**
 * Turns the light off.
 * @param [in]  port        The light port
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbio_light_off(pbio_port_t port);

/** @cond INTERNAL */
pbio_error_t _pbio_light_on(pbio_port_t port, pbio_color_t color, pbio_light_pattern_t pattern);
void _pbio_light_poll(uint32_t now);
void _pbio_light_set_user_mode(bool user_mode);
/** @endcond */

#else
static inline void _pbio_light_poll(uint32_t now) {
}
static inline void _pbio_light_set_user_mode(bool user_mode) {
}
static inline pbio_error_t _pbio_light_on(pbio_port_t port, pbio_color_t color, pbio_light_pattern_t pattern) {
    return PBIO_SUCCESS;
}
#endif // PBDRV_CONFIG_LIGHT

/** @cond */
// using macros for reduced code size
#define pbio_light_on(p, c) _pbio_light_on((p), (c), PBIO_LIGHT_PATTERN_NONE)
#define pbio_light_on_with_pattern(p, c, t) _pbio_light_on((p), (c), (t))
#define pbio_light_off(p) _pbio_light_on((p), PBIO_COLOR_NONE, PBIO_LIGHT_PATTERN_NONE)
/** @endcond */

#endif // _PBIO_LIGHT_H_

/** @}*/
