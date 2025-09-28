// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// OS-level hub built-in light matrix management.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <contiki.h>

#include <pbdrv/display.h>
#include <pbdrv/led.h>
#include <pbio/image.h>
#include <pbio/error.h>
#include <pbio/light_matrix.h>
#include <pbio/util.h>
#include <pbsys/config.h>
#include <pbsys/status.h>

#include <pbio/light_matrix.h>
#include "hmi.h"

#if PBSYS_CONFIG_HUB_LIGHT_MATRIX

// revisit, we will drop this in the next commit
pbio_light_matrix_t *pbsys_hub_light_matrix;

/**
 * Displays the idle UI. Has a square stop sign and selected slot on bottom row.
 *
 * @param brightness   Brightness (0--100%).
 */
static void pbsys_hub_light_matrix_show_idle_ui(uint8_t brightness) {
    for (uint8_t r = 0; r < pbsys_hub_light_matrix->size; r++) {
        for (uint8_t c = 0; c < pbsys_hub_light_matrix->size; c++) {
            bool is_on = r < 3 && c > 0 && c < 4;
            #if PBSYS_CONFIG_HMI_NUM_SLOTS
            is_on |= (r == 4 && c == pbsys_status_get_selected_slot());
            #endif
            pbio_light_matrix_set_pixel(pbsys_hub_light_matrix, r, c, is_on ? brightness : 0, false);
        }
    }
}

void pbsys_hub_light_matrix_update_program_slot(void) {
    pbsys_hub_light_matrix_show_idle_ui(100);
}

// Animation frame for on/off animation.
static uint32_t pbsys_hub_light_matrix_user_power_animation_next(pbio_light_animation_t *animation) {

    // Start at 2% and increment up to 100% in 7 steps.
    static uint8_t brightness = 2;
    static int8_t increment = 14;

    // Show the stop sign fading in/out.
    brightness += increment;
    pbsys_hub_light_matrix_show_idle_ui(brightness);

    // Stop at 100% and re-initialize so we can use this again for shutdown.
    if (brightness == 100 || brightness == 0) {
        pbio_light_animation_stop(&pbsys_hub_light_matrix->animation);
        brightness = 96;
        increment = -8;
    }
    return 40;
}

/**
 * Starts the power up and down animation. The first call makes it fade in the
 * stop sign. All subsequent calls are fade out.
 */
static void pbsys_hub_light_matrix_start_power_animation(void) {
    pbio_light_animation_init(&pbsys_hub_light_matrix->animation, pbsys_hub_light_matrix_user_power_animation_next);
    pbio_light_animation_start(&pbsys_hub_light_matrix->animation);
}

void pbsys_hub_light_matrix_init(void) {
    pbio_light_matrix_get_dev(0, 5, &pbsys_hub_light_matrix);
    pbsys_hub_light_matrix_start_power_animation();
}

void pbsys_hub_light_matrix_deinit(void) {
    // Starting it again continues it in reverse.
    pbsys_hub_light_matrix_start_power_animation();
}

/**
 * Clears the pixels needed for the run animation
 */
static void pbsys_hub_light_matrix_user_program_animation_clear(void) {
    for (uint8_t r = 0; r < 3; r++) {
        for (uint8_t c = 1; c < 4; c++) {
            pbio_light_matrix_set_pixel(pbsys_hub_light_matrix, r, c, 0, true);
        }
    }
}

// Animation frame for program running animation.
static uint32_t pbsys_hub_light_matrix_user_program_animation_next(pbio_light_animation_t *animation) {
    // The indexes of pixels to light up
    static const uint8_t indexes[] = { 1, 2, 3, 8, 13, 12, 11, 6 };

    // Each pixel has a repeating brightness pattern of the form /\_ through
    // which we can cycle in 256 steps.
    static uint8_t cycle = 0;

    for (size_t i = 0; i < PBIO_ARRAY_SIZE(indexes); i++) {
        // The pixels are spread equally across the pattern.
        uint8_t offset = cycle + i * (UINT8_MAX / PBIO_ARRAY_SIZE(indexes));
        uint8_t brightness = offset > 200 ? 0 : (offset < 100 ? offset : 200 - offset);

        // Set the brightness for this pixel
        pbio_light_matrix_set_pixel(pbsys_hub_light_matrix, indexes[i] / 5, indexes[i] % 5, brightness, false);
    }
    // This increment controls the speed of the pattern
    cycle += 9;

    return 40;
}

/**
 * Updates light matrix behavior when program is started.
 */
void pbsys_hub_light_matrix_handle_user_program_start(void) {

    // The user animation updates only a subset of pixels to save time,
    // so the rest must be cleared before it starts.
    pbsys_hub_light_matrix_user_program_animation_clear();
    pbio_light_animation_stop_all();
    pbio_light_animation_init(&pbsys_hub_light_matrix->animation, pbsys_hub_light_matrix_user_program_animation_next);
    pbio_light_animation_start(&pbsys_hub_light_matrix->animation);
}

#endif // PBSYS_CONFIG_HUB_LIGHT_MATRIX
