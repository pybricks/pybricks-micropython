// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <contiki.h>
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

#define YIELD(state)      \
    do {                                        \
        do_yield_now = 1;                       \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);    \
        if (do_yield_now) {                     \
            return PBIO_ERROR_AGAIN;            \
        }                                       \
    } while (0)

static pbio_error_t test_light_animation(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);

    static pbio_light_animation_t test_animation;
    pbio_light_animation_init(&test_animation, test_animation_next);

    // animation should not be started yet
    tt_want(!pbio_light_animation_is_started(&test_animation));

    // starting animation should set a timer at 0ms to call
    // next() after handling pending events
    pbio_light_animation_start(&test_animation);
    tt_want(pbio_light_animation_is_started(&test_animation));
    YIELD(state);
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 1);

    // next() should not be called again until after a delay
    pbio_test_clock_tick(TEST_ANIMATION_TIME - 2);
    YIELD(state);
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 1);
    pbio_test_clock_tick(1);
    YIELD(state);
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

    // stopping all animations stops the process
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
    PBIO_PT_THREAD_TEST_WITH_PBIO_OS(test_light_animation),
    END_OF_TESTCASES
};
