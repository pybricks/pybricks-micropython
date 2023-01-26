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

/**
 * Common test for pbio_servo_run_* functions.
 * @param [in]  pt      The Contiki proto-thread
 * @param [in]  name    The name of the test (i.e. __func__)
 * @param [in]  func    Callback for invoking pbio_servo_run_*
 */
static PT_THREAD(test_servo_run_func(struct pt *pt, const char *name, pbio_error_t (*func)(pbio_servo_t *servo))) {
    static pbdrv_motor_driver_dev_t *driver;
    static pbio_servo_t *servo;
    static struct timer timeout;

    pbdrv_motor_driver_init_manual();

    PT_BEGIN(pt);

    while (pbdrv_init_busy()) {
        PT_YIELD(pt);
    }

    timer_set(&timeout, 10 * 1000);

    tt_uint_op(pbdrv_motor_driver_get_dev(0, &driver), ==, PBIO_SUCCESS);

    process_start(&pbio_motor_process);
    tt_want(process_is_running(&pbio_motor_process));

    tt_uint_op(pbio_servo_get_servo(PBIO_PORT_ID_A, &servo), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(servo, PBIO_DIRECTION_CLOCKWISE, 1000, true), ==, PBIO_SUCCESS);
    tt_uint_op(func(servo), ==, PBIO_SUCCESS);

    while (!pbio_control_is_done(&servo->control) && !timer_expired(&timeout)) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }

    tt_want(pbio_control_is_done(&servo->control));

end:

    PT_END(pt);
}

static pbio_error_t test_servo_run_angle_func(pbio_servo_t *servo) {
    return pbio_servo_run_angle(servo, 500, 180, PBIO_CONTROL_ON_COMPLETION_HOLD);
}

static PT_THREAD(test_servo_run_angle(struct pt *pt)) {
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, test_servo_run_func(&child, __func__, test_servo_run_angle_func));

    PT_END(pt);
}

static pbio_error_t test_servo_run_time_func(pbio_servo_t *servo) {
    return pbio_servo_run_time(servo, 500, 1000, PBIO_CONTROL_ON_COMPLETION_HOLD);
}

static PT_THREAD(test_servo_run_time(struct pt *pt)) {
    static struct pt child;

    PT_BEGIN(pt);

    PT_SPAWN(pt, &child, test_servo_run_func(&child, __func__, test_servo_run_time_func));

    PT_END(pt);
}

struct testcase_t pbio_servo_tests[] = {
    PBIO_PT_THREAD_TEST(test_servo_run_angle),
    PBIO_PT_THREAD_TEST(test_servo_run_time),
    END_OF_TESTCASES
};
