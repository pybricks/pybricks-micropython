// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/motor_driver.h>
#include <pbio/control.h>
#include <pbio/error.h>
#include <pbio/logger.h>
#include <pbio/int_math.h>
#include <pbio/motor_process.h>
#include <pbio/servo.h>
#include <test-pbio.h>

#include "../src/processes.h"
#include "../drv/core.h"
#include "../drv/clock/clock_test.h"
#include "../drv/motor_driver/motor_driver_virtual_simulation.h"

static PT_THREAD(test_servo_basics(struct pt *pt)) {

    static struct timer timer;

    static pbio_servo_t *srv;

    // Start motor driver simulation process.
    pbdrv_motor_driver_init_manual();

    PT_BEGIN(pt);

    // Wait for motor simulation process to be ready.
    while (pbdrv_init_busy()) {
        PT_YIELD(pt);
    }

    // Start motor control process manually.
    pbio_motor_process_start();

    // Initialize the servo.
    tt_uint_op(pbio_servo_get_servo(PBIO_PORT_ID_A, &srv), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv, PBIO_DIRECTION_CLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);

    // Test running by angle.
    tt_uint_op(pbio_servo_run_angle(srv, 500, 180, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_control_is_done(&srv->control));

    // Test running for time.
    tt_uint_op(pbio_servo_run_time(srv, 500, 1000, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    pbio_test_sleep_ms(&timer, 500);
    tt_want(!pbio_control_is_done(&srv->control));
    pbio_test_sleep_until(pbio_control_is_done(&srv->control));

end:

    PT_END(pt);
}

static PT_THREAD(test_servo_stall(struct pt *pt)) {

    static struct timer timer;
    static pbio_servo_t *srv;

    static bool stalled;
    static uint32_t stall_duration;

    // Start motor driver simulation process.
    pbdrv_motor_driver_init_manual();

    PT_BEGIN(pt);

    // Wait for motor simulation process to be ready.
    while (pbdrv_init_busy()) {
        PT_YIELD(pt);
    }

    // Start motor control process manually.
    pbio_motor_process_start();

    // Initialize a servo that will stall.
    tt_uint_op(pbio_servo_get_servo(PBIO_PORT_ID_C, &srv), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv, PBIO_DIRECTION_CLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_run_forever(srv, -500), ==, PBIO_SUCCESS);

    // Should not be stalled at first.
    tt_uint_op(pbio_servo_is_stalled(srv, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(!stalled);

    // But after a we should be hitting the endpoint and stall.
    pbio_test_sleep_ms(&timer, 2000);
    tt_uint_op(pbio_servo_is_stalled(srv, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(stalled);

    // The same should be true after we turn around, which should immediately
    // unstall (evaluated on next control loop, so >= 5ms).
    tt_uint_op(pbio_servo_run_forever(srv, 500), ==, PBIO_SUCCESS);
    pbio_test_sleep_ms(&timer, 5);
    tt_uint_op(pbio_servo_is_stalled(srv, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(!stalled);

    // But after a we should be hitting the other endpoint and stall.
    pbio_test_sleep_ms(&timer, 2000);
    tt_uint_op(pbio_servo_is_stalled(srv, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(stalled);

end:

    PT_END(pt);
}

struct testcase_t pbio_servo_tests[] = {
    PBIO_PT_THREAD_TEST(test_servo_basics),
    PBIO_PT_THREAD_TEST(test_servo_stall),
    END_OF_TESTCASES
};
