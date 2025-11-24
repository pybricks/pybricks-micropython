// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/light.h>
#include <test-pbio.h>

#include "../src/light/color_light.h"
#include "../drv/clock/clock_test.h"

#define TEST_ANIMATION_TIME 10

static uint8_t test_light_set_hsv_call_count;
static uint16_t test_light_set_hsv_last_hue;
static uint16_t test_light_set_hsv_last_brightness;

static const uint16_t test_blink[] = {
    TEST_ANIMATION_TIME,
    TEST_ANIMATION_TIME,
    PBIO_COLOR_LIGHT_BLINK_END
};

static const pbio_color_compressed_hsv_t test_animation[] = {
    PBIO_COLOR_LIGHT_ANIMATION_CELL(PBIO_COLOR_HUE_CYAN, 100, 100),
    PBIO_COLOR_LIGHT_ANIMATION_CELL(PBIO_COLOR_HUE_MAGENTA, 100, 100),
    PBIO_COLOR_LIGHT_ANIMATION_END_HSV
};

static pbio_error_t test_light_set_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    test_light_set_hsv_call_count++;
    test_light_set_hsv_last_hue = hsv->h;
    test_light_set_hsv_last_brightness = pbio_color_hsv_get_v(hsv);
    return PBIO_SUCCESS;
}

static const pbio_color_light_funcs_t test_light_funcs = {
    .set_hsv = test_light_set_hsv,
};

static pbio_error_t test_color_light(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    static pbio_color_light_t test_light;
    pbio_color_light_init(&test_light, &test_light_funcs);

    // light on should trigger the callback with 100% brightness
    pbio_color_light_on(&test_light, PBIO_COLOR_GREEN);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_GREEN);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 100);

    // light off should trigger the callback with 0% brightness
    pbio_color_light_off(&test_light);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 0);

    // reset call count for next series of tests
    test_light_set_hsv_call_count = 0;

    // starting animation should call set_hsv() after handling pending events
    static const pbio_color_hsv_t hsv = { .h = PBIO_COLOR_HUE_BLUE, .s = 100, .v = 100 };
    pbio_color_light_start_blink_animation(&test_light, &hsv, test_blink);
    PBIO_OS_AWAIT_ONCE(state);

    tt_want_uint_op(test_light_set_hsv_call_count, ==, 1);
    // even blink cells turns the light on
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_BLUE);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 100);

    // set_hsv() should not be called again until after a delay and it should
    // receive the next hue in the list
    PBIO_OS_AWAIT_MS(state, &timer, TEST_ANIMATION_TIME);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 1);
    PBIO_OS_AWAIT_MS(state, &timer, 1);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 2);
    // odd blink cells turns the light off (so hue doesn't matter here)
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 0);

    // then the next animation update should wrap back to the start of the list
    PBIO_OS_AWAIT_MS(state, &timer, TEST_ANIMATION_TIME);
    PBIO_OS_AWAIT_ONCE(state);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 3);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_BLUE);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 100);

    // reset call count for next series of tests
    test_light_set_hsv_call_count = 0;

    // starting animation should call set_hsv() after handling pending events
    pbio_color_light_start_animation(&test_light, TEST_ANIMATION_TIME, test_animation);
    PBIO_OS_AWAIT_ONCE(state);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 1);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_CYAN);

    // set_hsv() should not be called again until after a delay and it should
    // receive the next hue in the list
    PBIO_OS_AWAIT_MS(state, &timer, TEST_ANIMATION_TIME);
    PBIO_OS_AWAIT_ONCE(state);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 2);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_MAGENTA);

    // then the next animation update should wrap back to the start of the list
    PBIO_OS_AWAIT_MS(state, &timer, TEST_ANIMATION_TIME);
    PBIO_OS_AWAIT_ONCE(state);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 3);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_CYAN);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

struct testcase_t pbio_color_light_tests[] = {
    PBIO_THREAD_TEST(test_color_light),
    END_OF_TESTCASES
};
