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
#include <pbio/angle.h>
#include <pbio/control.h>
#include <pbio/error.h>
#include <pbio/logger.h>
#include <pbio/int_math.h>
#include <pbio/motor_process.h>
#include <pbio/port_interface.h>
#include <pbio/servo.h>
#include <test-pbio.h>

#include "../drv/core.h"
#include "../drv/clock/clock_test.h"
#include "../drv/motor_driver/motor_driver_virtual_simulation.h"

static PT_THREAD(test_servo_basics(struct pt *pt)) {

    static struct timer timer;

    static int32_t angle;
    static int32_t start_angle;
    static int32_t speed;

    static pbio_servo_t *srv;
    static pbio_port_t *port;

    static uint32_t delay;

    PT_BEGIN(pt);

    // Give simulator some time to start reporting data.
    for (delay = 0; delay < 100; delay++) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }

    lego_device_type_id_t id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_A, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv, LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR, PBIO_DIRECTION_CLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);

    // Assert not moving to begin with.
    tt_uint_op(pbio_servo_get_state_user(srv, &start_angle, &speed), ==, PBIO_SUCCESS);
    tt_int_op(speed, ==, 0);

    // Test running BY angle.
    tt_uint_op(pbio_servo_run_angle(srv, 500, 180, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_control_is_done(&srv->control));
    tt_uint_op(pbio_servo_get_state_user(srv, &angle, &speed), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(angle, start_angle + 180, 5)); // Target should be close.
    tt_want(pbio_test_int_is_close(speed, 0, 100)); // Still allowed to move on completion.
    pbio_test_sleep_ms(&timer, 500);
    tt_uint_op(pbio_servo_get_state_user(srv, &angle, &speed), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(speed, 0, 50)); // Want further slowdown after holding.

    // Test running TO angle.
    tt_uint_op(pbio_servo_run_target(srv, 500, -90, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_control_is_done(&srv->control));
    tt_uint_op(pbio_servo_get_state_user(srv, &angle, &speed), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(angle, -90, 5)); // Target should be close.

    // Test running for time.
    tt_uint_op(pbio_servo_run_time(srv, 500, 1000, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    pbio_test_sleep_ms(&timer, 500);
    tt_want(!pbio_control_is_done(&srv->control));
    pbio_test_sleep_until(pbio_control_is_done(&srv->control));
    tt_want(pbio_test_int_is_close(speed, 0, 50));

end:

    PT_END(pt);
}

static PT_THREAD(test_servo_stall(struct pt *pt)) {

    static struct timer timer;
    static pbio_servo_t *srv;
    static pbio_port_t *port;

    static bool stalled;
    static uint32_t stall_duration;

    static uint32_t delay;

    PT_BEGIN(pt);

    // Give simulator some time to start reporting data.
    for (delay = 0; delay < 100; delay++) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }
    // Get legodev.
    lego_device_type_id_t id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_C, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv), ==, PBIO_SUCCESS);

    // Set up servo with given id.
    tt_uint_op(pbio_servo_setup(srv, LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR, PBIO_DIRECTION_CLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);

    // Start moving forever.
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

static PT_THREAD(test_servo_gearing(struct pt *pt)) {

    static struct timer timer;
    static pbio_servo_t *srv_basic;
    static pbio_servo_t *srv_geared;
    static pbio_port_t *port;
    static int32_t angle;
    static int32_t speed;
    static const int32_t geared_target = 90;
    static uint32_t delay;

    PT_BEGIN(pt);

    // Give simulator some time to start reporting data.
    for (delay = 0; delay < 100; delay++) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }

    // Initialize a servo without gears (1000 mdeg rotation per 1 deg output)
    lego_device_type_id_t id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_E, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv_basic), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv_basic, id, PBIO_DIRECTION_CLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_reset_angle(srv_basic, 0, false), ==, PBIO_SUCCESS);

    // Initialize a servo with 12:36 gears (3000 mdeg rotation per 1 deg output)
    id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_F, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv_geared), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv_geared, id, PBIO_DIRECTION_CLOCKWISE, 3000, true, 0), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_reset_angle(srv_geared, 0, false), ==, PBIO_SUCCESS);

    // Rotate both motors to +90 degrees.
    tt_uint_op(pbio_servo_run_target(srv_basic, 200, geared_target, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_run_target(srv_geared, 200, geared_target, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    srv_geared->control.settings.position_tolerance = 2000;
    pbio_test_sleep_until(pbio_control_is_done(&srv_basic->control) && pbio_control_is_done(&srv_geared->control));

    // For both, the user angle should match target.
    tt_uint_op(pbio_servo_get_state_user(srv_basic, &angle, &speed), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(angle, geared_target, 5));
    tt_uint_op(pbio_servo_get_state_user(srv_geared, &angle, &speed), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(angle, geared_target, 5));

    // Internally, the geared motor should have traveled trice as much.
    pbio_control_state_t state;
    pbio_angle_t angle_zero = (pbio_angle_t) {.rotations = 0, .millidegrees = 0};
    tt_uint_op(pbio_servo_get_state_control(srv_basic, &state), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(pbio_angle_diff_mdeg(&state.position, &angle_zero), geared_target * 1000, 5000));
    tt_uint_op(pbio_servo_get_state_control(srv_geared, &state), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(pbio_angle_diff_mdeg(&state.position, &angle_zero), geared_target * 3 * 1000, 5000));
    pbio_test_sleep_ms(&timer, 2000);

end:

    PT_END(pt);
}

struct testcase_t pbio_servo_tests[] = {
    PBIO_PT_THREAD_TEST_WITH_PBIO(test_servo_basics),
    PBIO_PT_THREAD_TEST_WITH_PBIO(test_servo_stall),
    PBIO_PT_THREAD_TEST_WITH_PBIO(test_servo_gearing),
    END_OF_TESTCASES
};
