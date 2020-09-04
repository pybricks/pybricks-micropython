// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// OS-level hub built-in light grid management.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/led.h>
#include <pbio/error.h>
#include <pbio/event.h>
#include <pbio/light_grid.h>
#include <pbio/util.h>
#include <pbsys/config.h>
#include <pbsys/status.h>

#include "../src/light/light_grid.h"

#if PBSYS_CONFIG_HUB_LIGHT_GRID

typedef struct {
    /** Struct for PBIO light grid implementation. */
    pbio_light_grid_t light_grid;
} pbsys_hub_light_grid_t;

static pbsys_hub_light_grid_t pbsys_hub_light_grid_instance;

/** The hub built-in light grid instance. */
pbio_light_grid_t *pbsys_hub_light_grid = &pbsys_hub_light_grid_instance.light_grid;

static pbio_error_t pbsys_hub_light_grid_set_pixel(pbio_light_grid_t *light_grid, uint8_t row, uint8_t col, uint8_t brightness) {
    // REVISIT: currently hub light grid is hard-coded as LED array at index 0
    // on all platforms
    pbdrv_led_array_dev_t *array;
    if (pbdrv_led_array_get_dev(0, &array) == PBIO_SUCCESS) {
        return pbdrv_led_array_set_brightness(array, row * light_grid->size + col, brightness);
    }

    return PBIO_SUCCESS;
}

static const pbio_light_grid_funcs_t pbsys_hub_light_grid_funcs = {
    .set_pixel = pbsys_hub_light_grid_set_pixel,
};

void pbsys_hub_light_grid_init() {
    pbio_light_grid_init(pbsys_hub_light_grid, 5, &pbsys_hub_light_grid_funcs);
}

// FIXME: compress / implement differently
static const uint8_t pbsys_hub_light_grid_user_program_animation_cells[] = {
    0, 0, 0, 0, 0, 10, 61, 99, 79, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 6, 53, 97, 85, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 3, 45, 94, 90, 39, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 38, 89, 94, 47, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 30, 84, 97, 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 23, 78, 99, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 2, 17, 71, 100, 70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 5, 12, 63, 99, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 9, 7, 56, 98, 83, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 14, 4, 48, 95, 89, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 20, 1, 40, 91, 93, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 27, 0, 33, 86, 97, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 34, 0, 25, 80, 99, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 42, 2, 19, 73, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 49, 4, 13, 66, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 57, 8, 8, 58, 98, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 65, 12, 5, 50, 96, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 72, 18, 2, 43, 92, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 79, 24, 0, 35, 87, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 85, 32, 0, 28, 82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 90, 39, 1, 21, 75, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 94, 47, 3, 15, 68, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 97, 55, 6, 10, 61, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 99, 62, 11, 6, 53, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 100, 70, 16, 3, 45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 99, 77, 22, 1, 37, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 98, 83, 29, 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 95, 88, 37, 1, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 91, 93, 44, 2, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 86, 96, 52, 5, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 80, 99, 60, 9, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 73, 100, 67, 14, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 66, 100, 75, 20, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 58, 98, 81, 27, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 51, 96, 87, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 43, 92, 92, 42, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 35, 88, 95, 50, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 28, 82, 98, 57, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 21, 76, 100, 65, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 15, 68, 100, 72, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

void pbsys_hub_light_grid_handle_event(process_event_t event, process_data_t data) {
    if (event == PBIO_EVENT_STATUS_SET && (pbsys_status_t)data == PBSYS_STATUS_USER_PROGRAM_RUNNING) {
        pbio_light_grid_start_animation(pbsys_hub_light_grid, pbsys_hub_light_grid_user_program_animation_cells,
            PBIO_ARRAY_SIZE(pbsys_hub_light_grid_user_program_animation_cells) / 25, 25);
    }
    if (event == PBIO_EVENT_STATUS_CLEARED && (pbsys_status_t)data == PBSYS_STATUS_USER_PROGRAM_RUNNING) {
        uint8_t rows[5] = {0};
        // 3x3 "stop sign" at top center of light grid
        rows[0] = rows[1] = rows[2] = 0b01110;
        // FIXME: should not be calling pbio user functions from pbsys.
        pbio_light_grid_set_rows(pbsys_hub_light_grid, rows);
    }
}

#endif // PBSYS_CONFIG_HUB_LIGHT_GRID
