// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/light.h>

#include "../src/light/color_light.h"

#define TEST_ANIMATION_TIME 10

static uint8_t test_light_set_hsv_call_count;
static uint16_t test_light_set_hsv_last_hue;
static uint16_t test_light_set_hsv_last_brightness;

static const uint16_t test_blink[] = {
    TEST_ANIMATION_TIME,
    TEST_ANIMATION_TIME,
    PBIO_COLOR_LIGHT_BLINK_END
};

static const pbio_color_hsv_t test_animation[] = {
    PBIO_COLOR_LIGHT_ANIMATION_CELL(PBIO_COLOR_HUE_CYAN, 100, 100),
    PBIO_COLOR_LIGHT_ANIMATION_CELL(PBIO_COLOR_HUE_MAGENTA, 100, 100),
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
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_GREEN);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 100);

    // light off should trigger the callback with 0% brightness
    pbio_color_light_off(&test_light);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 0);

    // reset call count for next series of tests
    test_light_set_hsv_call_count = 0;

    // starting animation should call set_hsv() synchonously
    static const pbio_color_hsv_t hsv = { .h = PBIO_COLOR_HUE_BLUE, .s = 100, .v = 100 };
    pbio_color_light_start_blink_animation(&test_light, &hsv, test_blink);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 1);
    // even blink cells turns the light on
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_BLUE);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 100);

    // set_hsv() should not be called again until after a delay and it should
    // receive the next hue in the list
    static struct timer timer;
    timer_set(&timer, TEST_ANIMATION_TIME);
    PT_WAIT_UNTIL(pt, test_light_set_hsv_call_count == 2);
    // odd blink cells turns the light off (so hue doesn't matter here)
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 0);
    tt_want(timer_expired(&timer));

    // then the next animation update should wrap back to the start of the list
    timer_set(&timer, TEST_ANIMATION_TIME);
    PT_WAIT_UNTIL(pt, test_light_set_hsv_call_count == 3);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_BLUE);
    tt_want_uint_op(test_light_set_hsv_last_brightness, ==, 100);
    tt_want(timer_expired(&timer));

    // reset call count for next series of tests
    test_light_set_hsv_call_count = 0;

    // starting animation should call set_hsv() synchonously
    pbio_color_light_start_animation(&test_light, TEST_ANIMATION_TIME, test_animation);
    tt_want_uint_op(test_light_set_hsv_call_count, ==, 1);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_CYAN);

    // set_hsv() should not be called again until after a delay and it should
    // receive the next hue in the list
    timer_set(&timer, TEST_ANIMATION_TIME);
    PT_WAIT_UNTIL(pt, test_light_set_hsv_call_count == 2);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_MAGENTA);
    tt_want(timer_expired(&timer));

    // then the next animation update should wrap back to the start of the list
    timer_set(&timer, TEST_ANIMATION_TIME);
    PT_WAIT_UNTIL(pt, test_light_set_hsv_call_count == 3);
    tt_want_uint_op(test_light_set_hsv_last_hue, ==, PBIO_COLOR_HUE_CYAN);
    tt_want(timer_expired(&timer));

    PT_END(pt);
}
