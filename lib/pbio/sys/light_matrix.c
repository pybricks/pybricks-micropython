// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// OS-level hub built-in light matrix management.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/led.h>
#include <pbio/error.h>
#include <pbio/event.h>
#include <pbio/light_matrix.h>
#include <pbio/util.h>
#include <pbsys/config.h>
#include <pbsys/status.h>

#include "../src/light/light_matrix.h"

#if PBSYS_CONFIG_HUB_LIGHT_MATRIX

typedef struct {
    /** Struct for PBIO light matrix implementation. */
    pbio_light_matrix_t light_matrix;
} pbsys_hub_light_matrix_t;

static pbsys_hub_light_matrix_t pbsys_hub_light_matrix_instance;

/** The hub built-in light matrix instance. */
pbio_light_matrix_t *pbsys_hub_light_matrix = &pbsys_hub_light_matrix_instance.light_matrix;

static pbio_error_t pbsys_hub_light_matrix_set_pixel(pbio_light_matrix_t *light_matrix, uint8_t row, uint8_t col, uint8_t brightness) {
    // REVISIT: currently hub light matrix is hard-coded as LED array at index 0
    // on all platforms
    pbdrv_led_array_dev_t *array;
    if (pbdrv_led_array_get_dev(0, &array) == PBIO_SUCCESS) {
        return pbdrv_led_array_set_brightness(array, row * light_matrix->size + col, brightness);
    }

    return PBIO_SUCCESS;
}

static const pbio_light_matrix_funcs_t pbsys_hub_light_matrix_funcs = {
    .set_pixel = pbsys_hub_light_matrix_set_pixel,
};

void pbsys_hub_light_matrix_init() {
    pbio_light_matrix_init(pbsys_hub_light_matrix, 5, &pbsys_hub_light_matrix_funcs);
}

static uint32_t pbsys_hub_light_matrix_user_program_animation_next(pbio_light_animation_t *animation) {
    // The indexes of pixels to light up
    static const uint8_t indexes[] = { 6, 7, 8, 13, 18, 17, 16, 11 };
    // sine wave for brightness
    static const uint8_t data[] = {
        0, 1, 2, 3, 5, 7, 10, 13, 17, 21, 25, 30, 35, 40, 45, 50,
        56, 61, 66, 71, 75, 80, 84, 88, 91, 94, 96, 98, 99, 100, 100, 100,
        99, 98, 96, 94, 91, 88, 84, 80, 75, 71, 66, 61, 56, 50, 45, 40,
        35, 30, 25, 21, 17, 13, 10, 7, 5, 3, 2, 1, 0, 0, 0, 0,
    };
    // keeps track of where we are in the animation
    static uint8_t offset = 0;

    pbdrv_led_array_dev_t *array;
    if (pbdrv_led_array_get_dev(0, &array) == PBIO_SUCCESS) {
        for (int i = 0; i < PBIO_ARRAY_SIZE(indexes); i++) {
            // Pixels are offset equally from each other in the data array so that
            // we get an animation that looks like the brightess is smoothly moving
            // from pixel to pixel.
            uint8_t brightness = data[(i * PBIO_ARRAY_SIZE(indexes) + offset++) % PBIO_ARRAY_SIZE(data)];
            pbdrv_led_array_set_brightness(array, indexes[i], brightness);
            // As long as PBIO_ARRAY_SIZE(data) is a multiple of the max offset (+1),
            // offset wraps around correctly when the addition overflows.
            _Static_assert((1 << (sizeof(offset) * 8)) % PBIO_ARRAY_SIZE(data) == 0, "needed for correct wraparound");
        }
    }

    return 100;
}

void pbsys_hub_light_matrix_handle_event(process_event_t event, process_data_t data) {
    if (event == PBIO_EVENT_STATUS_SET && (pbsys_status_t)data == PBSYS_STATUS_USER_PROGRAM_RUNNING) {
        for (uint8_t r = 0; r < pbsys_hub_light_matrix->size; r++) {
            for (uint8_t c = 0; c < pbsys_hub_light_matrix->size; c++) {
                pbsys_hub_light_matrix_set_pixel(pbsys_hub_light_matrix, r, c, 0);
            }
        }
        pbio_light_animation_init(&pbsys_hub_light_matrix->animation, pbsys_hub_light_matrix_user_program_animation_next);
        pbio_light_animation_start(&pbsys_hub_light_matrix->animation);
    }
    if (event == PBIO_EVENT_STATUS_CLEARED && (pbsys_status_t)data == PBSYS_STATUS_USER_PROGRAM_RUNNING) {
        // 3x3 "stop sign" at top center of light matrix
        for (uint8_t r = 0; r < pbsys_hub_light_matrix->size; r++) {
            for (uint8_t c = 0; c < pbsys_hub_light_matrix->size; c++) {
                uint8_t brightness = r < 3 && c > 0 && c < 4 ? 100: 0;
                pbsys_hub_light_matrix_set_pixel(pbsys_hub_light_matrix, r, c, brightness);
            }
        }
    }
}

#endif // PBSYS_CONFIG_HUB_LIGHT_MATRIX
