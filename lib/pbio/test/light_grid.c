// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/error.h>
#include <pbio/light_grid.h>

#include "../src/light/light_grid.h"

#define GRID_SIZE 3
#define INTERVAL 10

#define DATA_SIZE (GRID_SIZE * GRID_SIZE)
#define ROW_DATA(...) (const uint8_t[GRID_SIZE]) {__VA_ARGS__}
#define IMAGE_DATA(...) (const uint8_t[DATA_SIZE]) {__VA_ARGS__}

#define tt_want_light_grid_data(...) \
    tt_want_int_op(memcmp(test_light_grid_set_pixel_last_brightness, \
    (const uint8_t[DATA_SIZE]) {__VA_ARGS__}, DATA_SIZE), ==, 0)

static const uint8_t test_animation[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9,
    11, 12, 13, 14, 15, 16, 17, 18, 19,
};

static uint8_t test_light_grid_set_pixel_last_brightness[GRID_SIZE][GRID_SIZE];

void clock_override();
void clock_override_tick(clock_time_t ticks);

static void test_light_grid_reset() {
    memset(test_light_grid_set_pixel_last_brightness, 0, DATA_SIZE);
}

static pbio_error_t test_light_grid_set_pixel(pbio_light_grid_t *light_grid, uint8_t row, uint8_t col, uint8_t brightness) {
    test_light_grid_set_pixel_last_brightness[row][col] = brightness;
    return PBIO_SUCCESS;
}

static const pbio_light_grid_funcs_t test_light_grid_funcs = {
    .set_pixel = test_light_grid_set_pixel,
};

PT_THREAD(test_light_grid(struct pt *pt)) {
    PT_BEGIN(pt);

    clock_override();

    static pbio_light_grid_t test_light_grid;
    pbio_light_grid_init(&test_light_grid, 3, &test_light_grid_funcs);

    // ensure get size works
    tt_want_uint_op(pbio_light_grid_get_size(&test_light_grid), ==, GRID_SIZE);

    // set pixel should only set one pixel
    test_light_grid_reset();
    tt_want_uint_op(pbio_light_grid_set_pixel(&test_light_grid, 0, 0, 100), ==, PBIO_SUCCESS);
    tt_want_light_grid_data(100);

    tt_want_uint_op(pbio_light_grid_set_pixel(&test_light_grid,
        GRID_SIZE - 1, GRID_SIZE - 1, 100), ==, PBIO_SUCCESS);
    tt_want_light_grid_data(100, 0, 0, 0, 0, 0, 0, 0, 100);

    // out of bounds checking
    tt_want_uint_op(pbio_light_grid_set_pixel(&test_light_grid, GRID_SIZE, 0, 100), ==, PBIO_ERROR_INVALID_ARG);
    tt_want_light_grid_data(100, 0, 0, 0, 0, 0, 0, 0, 100);
    tt_want_uint_op(pbio_light_grid_set_pixel(&test_light_grid, 0, GRID_SIZE, 100), ==, PBIO_ERROR_INVALID_ARG);
    tt_want_light_grid_data(100, 0, 0, 0, 0, 0, 0, 0, 100);

    // bitwise mapping
    test_light_grid_reset();
    tt_want_uint_op(pbio_light_grid_set_rows(&test_light_grid, ROW_DATA(0b100, 0b010, 0b001)), ==, PBIO_SUCCESS);
    tt_want_light_grid_data(100, 0, 0, 0, 100, 0, 0, 0, 100);

    // bytewise mapping
    test_light_grid_reset();
    tt_want_uint_op(pbio_light_grid_set_image(&test_light_grid, IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_grid_data(1, 2, 3, 4, 5, 6, 7, 8, 9);

    // starting animation should call set_pixel() synchonously with the first cell data
    test_light_grid_reset();
    pbio_light_grid_start_animation(&test_light_grid, test_animation, 2, INTERVAL);
    tt_want_light_grid_data(1, 2, 3, 4, 5, 6, 7, 8, 9);

    // set_pixel() should not be called again until after a delay and it should
    // receive the next hue in the list
    clock_override_tick(INTERVAL - 1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_grid_set_pixel_last_brightness[0][0], ==, 1);
    clock_override_tick(1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_grid_set_pixel_last_brightness[0][0], !=, 1);
    tt_want_light_grid_data(11, 12, 13, 14, 15, 16, 17, 18, 19);

    // then the next animation update should wrap back to the start of the list
    clock_override_tick(INTERVAL - 1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_grid_set_pixel_last_brightness[0][0], ==, 11);
    clock_override_tick(1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_grid_set_pixel_last_brightness[0][0], !=, 11);
    tt_want_light_grid_data(1, 2, 3, 4, 5, 6, 7, 8, 9);

    // stopping the animation should not change any pixels
    test_light_grid_reset();
    pbio_light_grid_stop_animation(&test_light_grid);
    clock_override_tick(INTERVAL * 2);
    PT_YIELD(pt);
    tt_want_light_grid_data(0);

    PT_END(pt);
}
