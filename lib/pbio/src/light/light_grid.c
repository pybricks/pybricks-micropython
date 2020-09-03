// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_LIGHT_GRID

#include <stdbool.h>

#include <contiki.h>

#include <pbdrv/led.h>

#include <pbio/error.h>
#include <pbio/light_grid.h>

struct _pbio_light_grid_t {
    pbdrv_led_array_dev_t *led_array;
    /** Size of the grid (assumes grid is square) */
    uint8_t size;
    uint8_t number_of_frames;
    uint8_t frame_index;
    uint8_t interval;
    const uint8_t *frame_data;
};

PROCESS(pbio_light_grid_process, "light grid");
static pbio_light_grid_t _light_grid;

/**
 * Gets the light grid device.
 * @param [out] light_grid  The light grid instance.
 * @return                  ::PBIO_SUCCESS on success ::PBIO_ERROR_AGAIN if the
 *                          light grid is not ready for use yet or
 *                          ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is disabled.
 */
pbio_error_t pbio_light_grid_get_dev(pbio_light_grid_t **light_grid) {
    // REVISIT: currently only one known light grid
    pbio_light_grid_t *grid = &_light_grid;

    pbio_error_t err = pbdrv_led_array_get_dev(0, &grid->led_array);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    grid->size = 5;

    // Return device on success
    *light_grid = grid;

    return PBIO_SUCCESS;
}

/**
 * Get the size of the light grid.
 * @param [in]  light_grid  The light grid instance.
 * @return                  The grid size.
 */
uint8_t pbio_light_grid_get_size(pbio_light_grid_t *light_grid) {
    return light_grid->size;
}

/**
 * Sets the pixels of all rows bitwise.
 *
 * Each bit in a byte sets a pixel in a row, where 1 means a pixel is on and 0
 * is off. The least significant bit is the right-most pixel. This is the same
 * format as used by the micro:bit.
 *
 * @param [in]  light_grid  The light grid instance
 * @param [in]  rows        Array of size bytes. Each byte is one row, LSB right.
 * @return                  ::PBIO_SUCCESS on success or implementation-specific
 *                          error on failure.
 */
pbio_error_t pbio_light_grid_set_rows(pbio_light_grid_t *light_grid, const uint8_t *rows) {
    pbio_error_t err;
    uint8_t size = light_grid->size;

    // Loop through all rows i, starting at row 0 at the top.
    for (uint8_t i = 0; i < size; i++) {
        // Loop through all columns j, starting at col 0 on the left.
        for (uint8_t j = 0; j < size; j++) {
            // The pixel is on of the bit is high.
            bool on = rows[i] & (1 << (size - 1 - j));
            // Set the pixel.
            err = pbdrv_led_array_set_brightness(light_grid->led_array, i * size + j, on * 100);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }
    return PBIO_SUCCESS;
}

/**
 * Sets the pixel to a given brightness.
 * @param [in]  light_grid  The light grid instance
 * @param [in]  row         Row index (0 to size-1)
 * @param [in]  col         Column index (0 to size-1)
 * @param [in]  brightness  Brightness (0 to 100)
 * @return                  ::PBIO_SUCCESS on success or implementation-specific
 *                          error on failure.
 */
pbio_error_t pbio_light_grid_set_pixel(pbio_light_grid_t *light_grid, uint8_t row, uint8_t col, uint8_t brightness) {
    uint8_t size = light_grid->size;

    // Return early if the requested pixel is out of bounds
    if (row >= size || col >= size) {
        return PBIO_SUCCESS;
    }

    return pbdrv_led_array_set_brightness(light_grid->led_array, row * size + col, brightness);
}

/**
 * Sets the pixel to a given brightness.
 * @param [in]  light_grid  The light grid instance
 * @param [in]  image       Buffer of brightness values (0 to 100)
 * @return                  ::PBIO_SUCCESS on success or implementation-specific
 *                          error on failure.
 */
pbio_error_t pbio_light_grid_set_image(pbio_light_grid_t *light_grid, const uint8_t *image) {
    pbio_error_t err;
    uint8_t size = light_grid->size;

    for (uint8_t r = 0; r < size; r++) {
        for (uint8_t c = 0; c < size; c++) {
            err = pbio_light_grid_set_pixel(light_grid, r, c, image[r * size + c]);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }
    return PBIO_SUCCESS;
}

/**
 * Sets up the poller to display a series of frames
 * @param [in]  light_grid  The light grid instance
 * @param [in]  images      Buffer of buffer of brightness values (0 to 100)
 * @param [in]  frames      Number of images
 * @param [in]  interval    Time between subsequent images
 */
void pbio_light_grid_start_pattern(pbio_light_grid_t *light_grid, const uint8_t *images, uint8_t frames, uint32_t interval) {
    light_grid->number_of_frames = frames;
    light_grid->frame_index = 0;
    light_grid->interval = interval;
    light_grid->frame_data = images;

    process_start(&pbio_light_grid_process, NULL);
}

/**
 * Stops the pattern from updating further
 * @param [in]  light_grid  The light grid instance
 */
void pbio_light_grid_stop_pattern(pbio_light_grid_t *light_grid) {
    process_exit(&pbio_light_grid_process);
}

// FIXME: compress / implement differently
const uint8_t pbio_light_grid_sys_pattern[1000] = {
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

PROCESS_THREAD(pbio_light_grid_process, ev, data) {
    static pbio_light_grid_t *light_grid = &_light_grid;
    static struct etimer timer;

    PROCESS_BEGIN();

    etimer_set(&timer, clock_from_msec(light_grid->interval));

    for (;;) {
        // Current frame data
        uint8_t size = light_grid->size;
        const uint8_t *frame = light_grid->frame_data + size * size * light_grid->frame_index;

        // Display the frame
        pbio_light_grid_set_image(light_grid, frame);

        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));
        etimer_reset(&timer);

        // Move to next frame
        light_grid->frame_index = (light_grid->frame_index + 1) % light_grid->number_of_frames;
    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_LIGHT_GRID
