// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>
#include <test-pbio.h>

#include <pbio/error.h>
#include <pbio/light_matrix.h>

#include "../src/light/light_matrix.h"
#include "../drv/clock/clock_test.h"

#define MATRIX_SIZE 3
#define INTERVAL 10

#define DATA_SIZE (MATRIX_SIZE * MATRIX_SIZE)
#define ROW_DATA(...) (const uint8_t[MATRIX_SIZE]) {__VA_ARGS__}
#define IMAGE_DATA(...) (const uint8_t[DATA_SIZE]) {__VA_ARGS__}

#define tt_want_light_matrix_data(...) \
    tt_want_int_op(memcmp(test_light_matrix_set_pixel_last_brightness, \
    (const uint8_t[DATA_SIZE]) {__VA_ARGS__}, DATA_SIZE), ==, 0)

static const uint8_t test_animation[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9,
    11, 12, 13, 14, 15, 16, 17, 18, 19,
};

static uint8_t test_light_matrix_set_pixel_last_brightness[MATRIX_SIZE][MATRIX_SIZE];

static void test_light_matrix_reset(void) {
    memset(test_light_matrix_set_pixel_last_brightness, 0, DATA_SIZE);
}

static pbio_error_t test_light_matrix_set_pixel(pbio_light_matrix_t *light_matrix, uint8_t row, uint8_t col, uint8_t brightness) {
    test_light_matrix_set_pixel_last_brightness[row][col] = brightness;
    return PBIO_SUCCESS;
}

static const pbio_light_matrix_funcs_t test_light_matrix_funcs = {
    .set_pixel = test_light_matrix_set_pixel,
};

static PT_THREAD(test_light_matrix(struct pt *pt)) {
    PT_BEGIN(pt);

    static pbio_light_matrix_t test_light_matrix;
    pbio_light_matrix_init(&test_light_matrix, MATRIX_SIZE, &test_light_matrix_funcs);

    // ensure get size works
    tt_want_uint_op(pbio_light_matrix_get_size(&test_light_matrix), ==, MATRIX_SIZE);

    // set pixel should only set one pixel
    test_light_matrix_reset();
    tt_want_uint_op(pbio_light_matrix_set_pixel(&test_light_matrix, 0, 0, 100), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(100, 0, 0, 0, 0, 0, 0, 0, 0);

    tt_want_uint_op(pbio_light_matrix_set_pixel(&test_light_matrix,
        MATRIX_SIZE - 1, MATRIX_SIZE - 1, 100), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(100, 0, 0, 0, 0, 0, 0, 0, 100);

    // out of bounds checking
    tt_want_uint_op(pbio_light_matrix_set_pixel(&test_light_matrix, MATRIX_SIZE, 0, 100), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(100, 0, 0, 0, 0, 0, 0, 0, 100);
    tt_want_uint_op(pbio_light_matrix_set_pixel(&test_light_matrix, 0, MATRIX_SIZE, 100), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(100, 0, 0, 0, 0, 0, 0, 0, 100);

    // bitwise mapping
    test_light_matrix_reset();
    tt_want_uint_op(pbio_light_matrix_set_rows(&test_light_matrix, ROW_DATA(0b100, 0b010, 0b001)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(100, 0, 0, 0, 100, 0, 0, 0, 100);

    // bytewise mapping
    test_light_matrix_reset();
    tt_want_uint_op(pbio_light_matrix_set_image(&test_light_matrix,
        IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(1, 2, 3, 4, 5, 6, 7, 8, 9);

    // starting animation should call set_pixel() synchonously with the first cell data
    test_light_matrix_reset();
    pbio_light_matrix_start_animation(&test_light_matrix, test_animation, 2, INTERVAL);
    tt_want_light_matrix_data(1, 2, 3, 4, 5, 6, 7, 8, 9);

    // set_pixel() should not be called again until after a delay and it should
    // receive the next hue in the list
    pbio_test_clock_tick(INTERVAL - 1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_matrix_set_pixel_last_brightness[0][0], ==, 1);
    pbio_test_clock_tick(1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_matrix_set_pixel_last_brightness[0][0], !=, 1);
    tt_want_light_matrix_data(11, 12, 13, 14, 15, 16, 17, 18, 19);

    // then the next animation update should wrap back to the start of the list
    pbio_test_clock_tick(INTERVAL - 1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_matrix_set_pixel_last_brightness[0][0], ==, 11);
    pbio_test_clock_tick(1);
    PT_YIELD(pt);
    tt_want_int_op(test_light_matrix_set_pixel_last_brightness[0][0], !=, 11);
    tt_want_light_matrix_data(1, 2, 3, 4, 5, 6, 7, 8, 9);

    // stopping the animation should not change any pixels
    test_light_matrix_reset();
    pbio_light_matrix_stop_animation(&test_light_matrix);
    pbio_test_clock_tick(INTERVAL * 2);
    PT_YIELD(pt);
    tt_want_light_matrix_data(0);

    PT_END(pt);
}

static void test_light_matrix_rotation(void *env) {
    static pbio_light_matrix_t test_light_matrix;
    pbio_light_matrix_init(&test_light_matrix, MATRIX_SIZE, &test_light_matrix_funcs);

    // Default orientation has pixels in same order as underlying light array
    test_light_matrix_reset();
    tt_want_uint_op(pbio_light_matrix_set_image(&test_light_matrix,
        IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(
        1, 2, 3,
        4, 5, 6,
        7, 8, 9);

    // Check that other orientations work

    test_light_matrix_reset();
    pbio_light_matrix_set_orientation(&test_light_matrix, PBIO_GEOMETRY_SIDE_LEFT);
    tt_want_uint_op(pbio_light_matrix_set_image(&test_light_matrix,
        IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(
        3, 6, 9,
        2, 5, 8,
        1, 4, 7);

    test_light_matrix_reset();
    pbio_light_matrix_set_orientation(&test_light_matrix, PBIO_GEOMETRY_SIDE_BOTTOM);
    tt_want_uint_op(pbio_light_matrix_set_image(&test_light_matrix,
        IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(
        9, 8, 7,
        6, 5, 4,
        3, 2, 1);

    test_light_matrix_reset();
    pbio_light_matrix_set_orientation(&test_light_matrix, PBIO_GEOMETRY_SIDE_RIGHT);
    tt_want_uint_op(pbio_light_matrix_set_image(&test_light_matrix,
        IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(
        7, 4, 1,
        8, 5, 2,
        9, 6, 3);

    // front is same as top
    test_light_matrix_reset();
    pbio_light_matrix_set_orientation(&test_light_matrix, PBIO_GEOMETRY_SIDE_FRONT);
    tt_want_uint_op(pbio_light_matrix_set_image(&test_light_matrix,
        IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(
        1, 2, 3,
        4, 5, 6,
        7, 8, 9);

    // back is same as bottom
    test_light_matrix_reset();
    pbio_light_matrix_set_orientation(&test_light_matrix, PBIO_GEOMETRY_SIDE_BACK);
    tt_want_uint_op(pbio_light_matrix_set_image(&test_light_matrix,
        IMAGE_DATA(1, 2, 3, 4, 5, 6, 7, 8, 9)), ==, PBIO_SUCCESS);
    tt_want_light_matrix_data(
        9, 8, 7,
        6, 5, 4,
        3, 2, 1);
}

struct testcase_t pbio_light_matrix_tests[] = {
    PBIO_PT_THREAD_TEST(test_light_matrix),
    PBIO_TEST(test_light_matrix_rotation),
    END_OF_TESTCASES
};
