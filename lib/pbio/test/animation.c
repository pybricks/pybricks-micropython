// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdio.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include "../src/light/animation.h"

#define TEST_ANIMATION_TIME 10

PROCESS_NAME(pbio_light_animation_process);

static uint8_t test_animation_set_hsv_call_count;

static clock_time_t test_animation_next(pbio_light_animation_t *animation) {
    test_animation_set_hsv_call_count++;
    return TEST_ANIMATION_TIME;
}

PT_THREAD(test_light_animation(struct pt *pt)) {
    PT_BEGIN(pt);

    static pbio_light_animation_t test_animation;
    pbio_light_animation_init(&test_animation, test_animation_next);

    // process should not be running yet
    tt_want(!process_is_running(&pbio_light_animation_process))

    // starting animation should start process and call next() once synchonously
    pbio_light_animation_start(&test_animation);
    tt_want(process_is_running(&pbio_light_animation_process))
    tt_want_uint_op(test_animation_set_hsv_call_count, ==, 1);

    // next() should not be called again until after a delay
    static struct timer timer;
    timer_set(&timer, TEST_ANIMATION_TIME);
    PT_WAIT_UNTIL(pt, test_animation_set_hsv_call_count == 2);
    tt_want(timer_expired(&timer));

    // stopping the animation stops the process
    pbio_light_animation_stop(&test_animation);
    tt_want(!process_is_running(&pbio_light_animation_process))

    PT_END(pt);
}
