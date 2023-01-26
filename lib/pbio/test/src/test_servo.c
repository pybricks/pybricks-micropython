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
#include <pbio/servo.h>
#include <test-pbio.h>

#include "../src/processes.h"
#include "../drv/core.h"
#include "../drv/clock/clock_test.h"
#include "../drv/motor_driver/motor_driver_virtual_simulation.h"

static PT_THREAD(test_servo(struct pt *pt)) {
    static pbio_servo_t *srv;

    // Start motor driver simulation process.
    pbdrv_motor_driver_init_manual();

    PT_BEGIN(pt);

    // Wait for motor simulation process to be ready.
    while (pbdrv_init_busy()) {
        PT_YIELD(pt);
    }

    // Start motor control process.
    process_start(&pbio_motor_process);
    tt_want(process_is_running(&pbio_motor_process));

    // Initialize the servo.
    tt_uint_op(pbio_servo_get_servo(PBIO_PORT_ID_A, &srv), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv, PBIO_DIRECTION_CLOCKWISE, 1000, true), ==, PBIO_SUCCESS);

    // Test running by angle.
    tt_uint_op(pbio_servo_run_angle(srv, 500, 180, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    while (!pbio_control_is_done(&srv->control)) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }
    tt_want(pbio_control_is_done(&srv->control));

    // Test running for time.
    tt_uint_op(pbio_servo_run_time(srv, 500, 1000, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    while (!pbio_control_is_done(&srv->control)) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }
    tt_want(pbio_control_is_done(&srv->control));

end:

    PT_END(pt);
}

struct testcase_t pbio_servo_tests[] = {
    PBIO_PT_THREAD_TEST(test_servo),
    END_OF_TESTCASES
};
