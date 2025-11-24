// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <test-pbio.h>

#include <pbio/light_animation.h>

#include "../drv/clock/clock_test.h"

#define TEST_ANIMATION_TIME 10

static uint8_t test_animation_set_hsv_call_count;

static uint32_t test_animation_next(pbio_light_animation_t *animation) {
    test_animation_set_hsv_call_count++;
    return TEST_ANIMATION_TIME;
}

static pbio_error_t test_light_animation(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    static pbio_light_animation_t test_animation;
    pbio_light_animation_init(&test_animation, test_animation_next);

    // animation should not be started yet
    tt_want(!pbio_light_animation_is_started(&test_animation));

    // starting animation should set a timer at 0ms to call
    // next() after handling pending events
    pbio_light_animation_start(&test_animation);
    tt_want(pbio_light_animation_is_started(&test_animation));
    PBIO_OS_AWAIT_MS(state, &timer, 1);
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 1);

    // next() should not be called again until after a delay
    PBIO_OS_AWAIT_MS(state, &timer, TEST_ANIMATION_TIME - 1);
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 1);
    PBIO_OS_AWAIT_MS(state, &timer, 1);
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 2);

    // stopping the animation stops the animation
    pbio_light_animation_stop(&test_animation);
    tt_want(!pbio_light_animation_is_started(&test_animation));

    // exercise multiple animations for code coverage
    static pbio_light_animation_t test_animation2;
    pbio_light_animation_init(&test_animation2, test_animation_next);
    pbio_light_animation_start(&test_animation);
    pbio_light_animation_start(&test_animation2);
    tt_want(pbio_light_animation_is_started(&test_animation));
    tt_want(pbio_light_animation_is_started(&test_animation2));
    pbio_light_animation_stop(&test_animation);
    tt_want(!pbio_light_animation_is_started(&test_animation));
    tt_want(pbio_light_animation_is_started(&test_animation2));
    pbio_light_animation_stop(&test_animation2);
    tt_want(!pbio_light_animation_is_started(&test_animation));
    tt_want(!pbio_light_animation_is_started(&test_animation2));

    // stop all animations
    pbio_light_animation_start(&test_animation);
    pbio_light_animation_start(&test_animation2);
    tt_want(pbio_light_animation_is_started(&test_animation));
    tt_want(pbio_light_animation_is_started(&test_animation2));
    pbio_light_animation_stop_all();
    tt_want(!pbio_light_animation_is_started(&test_animation));
    tt_want(!pbio_light_animation_is_started(&test_animation2));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

struct testcase_t pbio_light_animation_tests[] = {
    PBIO_THREAD_TEST(test_light_animation),
    END_OF_TESTCASES
};
