// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Functions to control color lights such as RGB LEDs.

#include <pbio/config.h>

#if PBIO_CONFIG_LIGHT

#include <contiki.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/light.h>
#include <pbio/util.h>

#include "animation.h"
#include "color_light.h"

/**
 * Initializes the required fields of a color light data structure.
 * @param [in]  light       The light instance
 * @param [in]  funcs       The callbacks
 */
void pbio_color_light_init(pbio_color_light_t *light, const pbio_color_light_funcs_t *funcs) {
    light->funcs = funcs;
}

/**
 * Turns the light on with specifed color and brightness.
 *
 * Some lights may not be capable of displaying all colors. Some lights may not
 * have adjustable brightness.
 *
 * The V in HSV will be the brightness of the LED. Colors like brown and gray
 * are not possible since lights emit light rather than reflect it.
 *
 * @param [in]  light       The light instance
 * @param [in]  hsv         The color and brightness
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_NO_DEV if the light is not connected
 */
pbio_error_t pbio_color_light_on_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    return light->funcs->set_hsv(light, hsv);
}

/**
 * Turns the light on with specified color.
 *
 * Some lights may not be capable of displaying all colors.
 *
 * ::PBIO_COLOR_BLACK or ::PBIO_COLOR_NONE will turn the light off instead of on.
 *
 * Colors like ::PBIO_COLOR_BROWN and ::PBIO_COLOR_GRAY are not possible since
 * lights emit light rather than reflect it and will appear as dim orange and
 * dim white instead.
 *
 * @param [in]  light       The light instance
 * @param [in]  color       The color
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_NO_DEV if the light is not connected
 */
pbio_error_t pbio_color_light_on(pbio_color_light_t *light, pbio_color_t color) {
    pbio_color_hsv_t hsv;
    pbio_color_to_hsv(color, &hsv);
    return pbio_color_light_on_hsv(light, &hsv);
}

/**
 * Turns the light off.
 * @param [in]  light       The light instance
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbio_color_light_off(pbio_color_light_t *light) {
    pbio_color_hsv_t hsv = { 0 };
    return pbio_color_light_on_hsv(light, &hsv);
}

static clock_time_t pbio_color_light_animation_next(pbio_light_animation_t *animation) {
    pbio_color_light_t *light = PBIO_CONTAINER_OF(animation, pbio_color_light_t, animation);

    const pbio_color_light_animation_cell_t *cell = &light->cells[light->current_cell++];

    // if we have reached the array terminator, start back at the beginning
    if (cell->duration == 0) {
        cell = &light->cells[0];
        light->current_cell = 1;
    }

    pbio_color_light_on_hsv(light, &cell->hsv);
    return clock_from_msec(cell->duration);
}

/**
 * Starts animating the light.
 *
 * This will start a background timer to animate the lights using the information
 * in @p cells. The data in @p cells must remain valid until pbio_color_light_stop_animation()
 * is called.
 *
 * The animation must be stopped by calling pbio_color_light_stop_animation()
 * before using other color light functions including calling pbio_color_light_start_animation()
 * again, otherwise the animation will continue to update the light.
 *
 * @param [in]  light       The light instance
 * @param [in]  cells       Array of up to 256 animation cells ending with ::PBIO_COLOR_LIGHT_ANIMATION_END
 */
void pbio_color_light_start_animation(pbio_color_light_t *light, const pbio_color_light_animation_cell_t *cells) {
    pbio_light_animation_init(&light->animation, pbio_color_light_animation_next);
    light->cells = cells;
    light->current_cell = 0;
    pbio_light_animation_start(&light->animation);
}

/**
 * Stops animating the light.
 *
 * See pbio_color_light_start_animation().
 *
 * @param [in]  light       The light instance
 */
void pbio_color_light_stop_animation(pbio_color_light_t *light) {
    pbio_light_animation_stop(&light->animation);
}

#endif // PBIO_CONFIG_LIGHT
