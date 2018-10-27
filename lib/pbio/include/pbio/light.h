/**
 * \addtogroup Light Light control functions
 * @{
 */

#ifndef _PBIO_LIGHT_H_
#define _PBIO_LIGHT_H_

#include <stdbool.h>

#include <pbio/error.h>
#include <pbio/port.h>

/**
 * Light color values.
 */
typedef enum {
    PBIO_LIGHT_COLOR_NONE      = 0,    /**< The light is off */    
    PBIO_LIGHT_COLOR_PINK      = 1,    /**< TODO: firmware #14 */
    PBIO_LIGHT_COLOR_PURPLE    = 2,    /**< The light is purple */
    PBIO_LIGHT_COLOR_BLUE      = 3,    /**< The light is blue */
    PBIO_LIGHT_COLOR_CYAN      = 4,    /**< The light is cyan (blue with some green) */
    PBIO_LIGHT_COLOR_TURQUOISE = 5,    /**< The light is turquoise (green with some blue) */
    PBIO_LIGHT_COLOR_GREEN     = 6,    /**< The light is green */    
    PBIO_LIGHT_COLOR_YELLOW    = 7,    /**< The light is yellow */    
    PBIO_LIGHT_COLOR_ORANGE    = 8,    /**< The light is orange */    
    PBIO_LIGHT_COLOR_RED       = 9,    /**< The light is red */    
    PBIO_LIGHT_COLOR_WHITE     = 10,   /**< The light is white */    
} pbio_light_color_t;

/**
 * Light patterns.
 */
typedef enum {
    PBIO_LIGHT_PATTERN_NONE,    /**< The light does not change */
    PBIO_LIGHT_PATTERN_FLASH,   /**< The light flashes */
    PBIO_LIGHT_PATTERN_BREATHE, /**< The light breathes */
} pbio_light_pattern_t;

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
pbio_error_t pbio_light_on(pbio_port_t port, pbio_light_color_t color);

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
pbio_error_t pbio_light_on_with_pattern(pbio_port_t port, pbio_light_color_t color, pbio_light_pattern_t pattern);

/**
 * Turns the light off.
 * @param [in]  port        The light port
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbio_light_off(pbio_port_t port);

/** @cond INTERNAL */
pbio_error_t _pbio_light_on(pbio_port_t port, pbio_light_color_t color, pbio_light_pattern_t pattern);
void _pbio_light_poll(uint32_t now);
void _pbio_light_set_user_mode(bool user_mode);
/** @endcond */

/** @cond */
// using macros for reduced code size
#define pbio_light_on(p, c) _pbio_light_on((p), (c), PBIO_LIGHT_PATTERN_NONE)
#define pbio_light_on_with_pattern(p, c, t) _pbio_light_on((p), (c), (t))
#define pbio_light_off(p) _pbio_light_on((p), PBIO_LIGHT_COLOR_NONE, PBIO_LIGHT_PATTERN_NONE)
/** @endcond */

#endif // _PBIO_LIGHT_H_
