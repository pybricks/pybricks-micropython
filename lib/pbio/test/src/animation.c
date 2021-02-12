// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <test-pbio.h>

#include "../src/light/animation.h"

#define TEST_ANIMATION_TIME 10

PROCESS_NAME(pbio_light_animation_process);

static uint8_t test_animation_set_hsv_call_count;

static clock_time_t test_animation_next(pbio_light_animation_t *animation) {
    test_animation_set_hsv_call_count++;
    return TEST_ANIMATION_TIME;
}

static PT_THREAD(test_light_animation(struct pt *pt)) {
    PT_BEGIN(pt);

    static pbio_light_animation_t test_animation;
    pbio_light_animation_init(&test_animation, test_animation_next);

    // process should not be running yet
    tt_want(!process_is_running(&pbio_light_animation_process));
    tt_want(!pbio_light_animation_is_started(&test_animation));

    // starting animation should start process and call next() once synchonously
    pbio_light_animation_start(&test_animation);
    tt_want(pbio_light_animation_is_started(&test_animation));
    tt_want(process_is_running(&pbio_light_animation_process));
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 1);

    // next() should not be called again until after a delay
    clock_tick(TEST_ANIMATION_TIME - 1);
    PT_YIELD(pt);
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 1);
    clock_tick(1);
    PT_YIELD(pt);
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 2);

    // stopping the animation stops the process
    pbio_light_animation_stop(&test_animation);
    tt_want(!pbio_light_animation_is_started(&test_animation));
    tt_want(!process_is_running(&pbio_light_animation_process));

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
    tt_want(process_is_running(&pbio_light_animation_process));
    pbio_light_animation_stop(&test_animation2);
    tt_want(!pbio_light_animation_is_started(&test_animation));
    tt_want(!pbio_light_animation_is_started(&test_animation2));
    tt_want(!process_is_running(&pbio_light_animation_process));

    // stopping all animations stops the process
    pbio_light_animation_start(&test_animation);
    pbio_light_animation_start(&test_animation2);
    tt_want(pbio_light_animation_is_started(&test_animation));
    tt_want(pbio_light_animation_is_started(&test_animation2));
    tt_want(process_is_running(&pbio_light_animation_process));
    pbio_light_animation_stop_all();
    tt_want(!pbio_light_animation_is_started(&test_animation));
    tt_want(!pbio_light_animation_is_started(&test_animation2));
    tt_want(!process_is_running(&pbio_light_animation_process));

    PT_END(pt);
}

struct testcase_t pbio_light_animation_tests[] = {
    PBIO_PT_THREAD_TEST(test_light_animation),
    END_OF_TESTCASES
};
