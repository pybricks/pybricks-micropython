// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdio.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include "../src/light/color_light.h"

#define TEST_ANIMATION_TIME 10
#define TEST_HUE_1 30
#define TEST_HUE_2 60

static uint8_t test_light_set_hsv_call_count;
static uint16_t test_light_set_hsv_last_hue;
static uint16_t test_light_set_hsv_last_brightness;

static const pbio_color_light_animation_cell_t test_animation[] = {
    PBIO_COLOR_LIGHT_ANIMATION_CELL(TEST_HUE_1, 100, 100, TEST_ANIMATION_TIME),
    PBIO_COLOR_LIGHT_ANIMATION_CELL(TEST_HUE_2, 100, 100, TEST_ANIMATION_TIME),
    PBIO_COLOR_LIGHT_ANIMATION_END
};

static pbio_error_t test_light_set_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    test_light_set_hsv_call_count++;
    test_light_set_hsv_last_hue = hsv->h;
    test_light_set_hsv_last_brightness = hsv->v;
    return PBIO_SUCCESS;
}

static const pbio_color_light_funcs_t test_light_funcs = {
    .set_hsv = test_light_set_hsv,
};

PT_THREAD(test_color_light(struct pt *pt)) {
    PT_BEGIN(pt);

    static pbio_color_light_t test_light;
    pbio_color_light_init(&test_light, &test_light_funcs);

    // light on should trigger the callback with 100% brightness
    pbio_color_light_on(&test_light, PBIO_COLOR_GREEN);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, 120); // green is 120 degrees
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 100);

    // light off should trigger the callback with 0% brightness
    pbio_color_light_off(&test_light);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 0);

    // reset call count for next series of tests
    test_light_set_hsv_call_count = 0;

    // starting animation should call set_hsv() synchonously
    pbio_color_light_start_animation(&test_light, test_animation);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 1);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, TEST_HUE_1);

    // set_hsv() should not be called again until after a delay and it should
    // receive the next hue in the list
    static struct timer timer;
    timer_set(&timer, TEST_ANIMATION_TIME);
    PT_WAIT_UNTIL(pt, test_light_set_hsv_call_count == 2);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, TEST_HUE_2);
    tt_want(timer_expired(&timer));

    // then the next animation update should wrap back to the start of the list
    timer_set(&timer, TEST_ANIMATION_TIME);
    PT_WAIT_UNTIL(pt, test_light_set_hsv_call_count == 3);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, TEST_HUE_1);
    tt_want(timer_expired(&timer));

    pbio_color_light_stop_animation(&test_light);

    PT_END(pt);
}
