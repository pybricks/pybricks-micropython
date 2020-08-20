// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_LIGHTGRID

#include <contiki.h>

#include <stdbool.h>

#include <pbdrv/pwm.h>

#include <pbio/error.h>
#include <pbio/lightgrid.h>

struct _pbio_lightgrid_t {
    pbdrv_pwm_dev_t *pwm;
    const pbdrv_lightgrid_platform_data_t *data;
    uint32_t last_poll;
    uint8_t number_of_frames;
    uint8_t frame_index;
    uint8_t interval;
    const uint8_t *frame_data;
};

static pbio_lightgrid_t _lightgrid;

pbio_error_t pbio_lightgrid_get_dev(pbio_lightgrid_t **lightgrid) {

    pbio_lightgrid_t *grid = &_lightgrid;

    // Get data
    grid->data = &pbdrv_lightgrid_platform_data;

    // Get PWM device
    pbio_error_t err = pbdrv_pwm_get_dev(grid->data->id, &grid->pwm);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return device on success
    *lightgrid = grid;

    return PBIO_SUCCESS;
}

uint8_t pbio_lightgrid_get_size(pbio_lightgrid_t *lightgrid) {
    return lightgrid->data->size;
}

// Each byte sets a row, where 1 means a pixel is on and 0 is off. Least significant bit is on the right.
// This is the same format as used by the micro:bit.
pbio_error_t pbio_lightgrid_set_rows(pbio_lightgrid_t *lightgrid, const uint8_t *rows) {

    pbio_error_t err;
    uint8_t size = lightgrid->data->size;

    // Loop through all rows i, starting at row 0 at the top.
    for (uint8_t i = 0; i < size; i++) {
        // Loop through all columns j, starting at col 0 on the left.
        for (uint8_t j = 0; j < size; j++) {
            // The pixel is on of the bit is high.
            bool on = rows[i] & (1 << (size - 1 - j));
            // Set the pixel.
            err = pbdrv_pwm_set_duty(lightgrid->pwm, lightgrid->data->channels[i * size + j], on * UINT16_MAX);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }
    return PBIO_SUCCESS;
}

// Sets one pixel to an approximately perceived brightness of 0--100%
pbio_error_t pbio_lightgrid_set_pixel(pbio_lightgrid_t *lightgrid, uint8_t row, uint8_t col, uint8_t brightness) {

    uint8_t size = lightgrid->data->size;

    // Return early if the requested pixel is out of bounds
    if (row >= size || col >= size) {
        return PBIO_SUCCESS;
    }

    // Scale brightness quadratically from 0 to UINT16_MAX
    int32_t duty = ((int32_t)brightness) * brightness * UINT16_MAX / 10000;

    return pbdrv_pwm_set_duty(lightgrid->pwm, lightgrid->data->channels[row * size + col], duty);
}

// Displays an image on the screen
pbio_error_t pbio_lightgrid_set_image(pbio_lightgrid_t *lightgrid, const uint8_t *image) {

    pbio_error_t err;
    uint8_t size = lightgrid->data->size;

    for (uint8_t r = 0; r < size; r++) {
        for (uint8_t c = 0; c < size; c++) {
            err = pbio_lightgrid_set_pixel(lightgrid, r, c, image[r * size + c]);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }
    return PBIO_SUCCESS;
}

void pbio_lightgrid_stop_pattern(pbio_lightgrid_t *lightgrid) {
    lightgrid->number_of_frames = 0;
}

pbio_error_t pbio_lightgrid_start_pattern(pbio_lightgrid_t *lightgrid, const uint8_t *images, uint8_t frames, uint32_t interval) {
    lightgrid->number_of_frames = frames;
    lightgrid->frame_index = 0;
    lightgrid->interval = interval;
    lightgrid->frame_data = images;
    lightgrid->last_poll = clock_time();

    // Start with the first frame
    return pbio_lightgrid_set_image(lightgrid, images);
}

// FIXME: compress / implement differently
const uint8_t pbio_lightgrid_sys_pattern[1000] = {
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

// TODO: Convert to contiki process
void _pbio_lightgrid_poll(uint32_t now) {

    pbio_lightgrid_t *lightgrid = &_lightgrid;

    // Poll only if there are frames to do
    if (lightgrid->number_of_frames > 1) {

        // Check if we are past the next sample yet
        if (now - lightgrid->last_poll >= lightgrid->interval) {

            // Bump the poll time and frame index
            lightgrid->last_poll += lightgrid->interval;
            lightgrid->frame_index = (lightgrid->frame_index + 1) % lightgrid->number_of_frames;

            // Current frame
            const uint8_t *frame = lightgrid->frame_data + lightgrid->data->size * lightgrid->data->size * lightgrid->frame_index;

            // Display the frame
            pbio_lightgrid_set_image(lightgrid, frame);
        }
    }

}

#endif // PBIO_CONFIG_LIGHTGRID
