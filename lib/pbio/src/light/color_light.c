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
    pbio_light_animation_init(&light->animation, NULL);
}

/**
 * Stops animating the light. Has no effect if animation is not started.
 *
 * @param [in]  light       The light instance
 */
static void pbio_color_light_stop_animation(pbio_color_light_t *light) {
    if (pbio_light_animation_is_started(&light->animation)) {
        pbio_light_animation_stop(&light->animation);
    }
}

/**
 * Turns the light on with specified color and brightness.
 *
 * Some lights may not be capable of displaying all colors. Some lights may not
 * have adjustable brightness.
 *
 * The V in HSV will be the brightness of the LED. Colors like brown and gray
 * are not possible since lights emit light rather than reflect it.
 *
 * If an animation is running in the background, it will be stopped.
 *
 * @param [in]  light       The light instance
 * @param [in]  hsv         The color and brightness
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_NO_DEV if the light is not connected
 */
pbio_error_t pbio_color_light_on_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    pbio_color_light_stop_animation(light);
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
 * If an animation is running in the background, it will be stopped.
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
 *
 * If an animation is running in the background, it will be stopped.
 *
 * @param [in]  light       The light instance
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_INVALID_PORT if port is not a valid port
 *                          ::PBIO_ERROR_NO_DEV if port is valid but light is not connected
 */
pbio_error_t pbio_color_light_off(pbio_color_light_t *light) {
    pbio_color_hsv_t hsv = { 0 };
    return pbio_color_light_on_hsv(light, &hsv);
}

static uint32_t pbio_color_light_blink_next(pbio_light_animation_t *animation) {
    pbio_color_light_t *light = PBIO_CONTAINER_OF(animation, pbio_color_light_t, animation);

    const uint16_t *cells = light->interval_cells;
    const uint16_t *interval = &cells[light->current_cell++];

    // if we have reached the array terminator, start back at the beginning
    if (*interval == PBIO_COLOR_LIGHT_BLINK_END) {
        interval = &cells[0];
        light->current_cell = 1;
    }

    pbio_color_hsv_t off = { 0 };
    light->funcs->set_hsv(light, light->current_cell % 2 ? &light->hsv : &off);
    return *interval;
}

/**
 * Starts blinking the light.
 *
 * This will start a background timer to blink the lights using the durations
 * in @p cells. The data in @p cells must remain valid until animation is stopped.
 *
 * The light will alternate between on and off for the durations given by @p
 * cells (i.e. even array indexes will be the on duration and odd array indexes
 * will be the off duration).
 *
 * If another animation is running in the background, it will be stopped and
 * replaced with this one.
 *
 * @param [in]  light       The light instance.
 * @param [in]  hsv         The HSV value for when the light is on.
 * @param [in]  cells       Array of up to 65536 duration values ending with ::PBIO_COLOR_LIGHT_BLINK_END.
 */
void pbio_color_light_start_blink_animation(pbio_color_light_t *light, const pbio_color_hsv_t *hsv, const uint16_t *cells) {
    pbio_color_light_stop_animation(light);
    pbio_light_animation_init(&light->animation, pbio_color_light_blink_next);
    light->hsv = *hsv;
    light->interval_cells = cells;
    light->current_cell = 0;
    pbio_light_animation_start(&light->animation);
}

static uint32_t pbio_color_light_animate_next(pbio_light_animation_t *animation) {
    pbio_color_light_t *light = PBIO_CONTAINER_OF(animation, pbio_color_light_t, animation);

    const pbio_color_compressed_hsv_t *cells = light->hsv_cells;
    const pbio_color_compressed_hsv_t *cell = &cells[light->current_cell++];

    // if we have reached the array terminator, start back at the beginning
    if (cell->v == UINT8_MAX) {
        cell = &cells[0];
        light->current_cell = 1;
    }

    pbio_color_hsv_t hsv;
    pbio_color_hsv_expand(cell, &hsv);
    light->funcs->set_hsv(light, &hsv);
    return light->interval;
}

/**
 * Starts animating the light.
 *
 * This will start a background timer to animate the lights using the information
 * in @p cells. The data in @p cells must remain valid until animation is stopped.
 *
 * If another animation is running in the background, it will be stopped and
 * replaced with this one.
 *
 * @param [in]  light       The light instance
 * @param [in]  interval    The the time intervale between animaction cells in milliseconds
 * @param [in]  cells       Array of up to 65536 animation cells ending with ::PBIO_COLOR_LIGHT_ANIMATION_END
 */
void pbio_color_light_start_animation(pbio_color_light_t *light, uint16_t interval, const pbio_color_compressed_hsv_t *cells) {
    pbio_color_light_stop_animation(light);
    pbio_light_animation_init(&light->animation, pbio_color_light_animate_next);
    light->interval = interval;
    light->hsv_cells = cells;
    light->current_cell = 0;
    pbio_light_animation_start(&light->animation);
}

#endif // PBIO_CONFIG_LIGHT
