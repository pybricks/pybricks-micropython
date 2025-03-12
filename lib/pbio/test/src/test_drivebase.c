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
#include <pbio/drivebase.h>
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

static PT_THREAD(test_drivebase_basics(struct pt *pt)) {

    static struct timer timer;

    static pbio_servo_t *srv_left;
    static pbio_servo_t *srv_right;
    static pbio_drivebase_t *db;
    static pbio_port_t *port;

    static int32_t drive_distance;
    static int32_t drive_speed;
    static int32_t drive_acceleration;
    static int32_t drive_deceleration;
    static int32_t turn_angle_start;
    static int32_t turn_angle;
    static int32_t turn_rate;
    static int32_t turn_acceleration;
    static int32_t turn_deceleration;

    static bool stalled;
    static uint32_t stall_duration;

    static pbio_dcmotor_actuation_t actuation;
    static int32_t voltage;

    static uint32_t delay;

    PT_BEGIN(pt);

    // Give simulator some time to start reporting data.
    for (delay = 0; delay < 100; delay++) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }

    // Initialize the servos.

    lego_device_type_id_t id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_A, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv_left), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv_left, id, PBIO_DIRECTION_COUNTERCLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_B, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv_right), ==, PBIO_SUCCESS);

    tt_uint_op(pbio_servo_setup(srv_right, id, PBIO_DIRECTION_CLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);

    // Set up the drivebase.
    tt_uint_op(pbio_drivebase_get_drivebase(&db, srv_left, srv_right, 56000, 112000), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle_start, &turn_rate), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_drivebase_is_stalled(db, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(!stalled);

    // Get current settings and change them.
    tt_uint_op(pbio_drivebase_get_drive_settings(db,
        &drive_speed,
        &drive_acceleration,
        &drive_deceleration,
        &turn_rate,
        &turn_acceleration,
        &turn_deceleration), ==, PBIO_SUCCESS);

    // Try to set invalid settings.
    tt_uint_op(pbio_drivebase_set_drive_settings(db,
        drive_speed,
        drive_acceleration * 100,
        drive_deceleration,
        turn_rate,
        turn_acceleration,
        turn_deceleration), ==, PBIO_ERROR_INVALID_ARG);

    // Try to set valid settings.
    tt_uint_op(pbio_drivebase_set_drive_settings(db,
        200,
        drive_acceleration * 2,
        drive_deceleration,
        turn_rate,
        turn_acceleration,
        turn_deceleration), ==, PBIO_SUCCESS);

    // Drive straight for a distance and coast smart.
    tt_uint_op(pbio_drivebase_drive_straight(db, 1000, PBIO_CONTROL_ON_COMPLETION_COAST_SMART), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_drivebase_is_done(db));

    // Target should be stationary and close to target.
    pbio_test_sleep_ms(&timer, 200);
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle, &turn_rate), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(drive_distance, 1000, 30));
    tt_want(pbio_test_int_is_close(drive_speed, 0, 50));
    tt_want(pbio_test_int_is_close(turn_angle, turn_angle_start, 5));
    tt_want(pbio_test_int_is_close(turn_rate, 0, 10));

    // Drive straight for a distance and keep driving.
    tt_uint_op(pbio_drivebase_drive_straight(db, 1000, PBIO_CONTROL_ON_COMPLETION_CONTINUE), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_drivebase_is_done(db));

    // Target should be moving at given speed and close to target.
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle, &turn_rate), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(drive_distance, 2000, 20));
    tt_want(pbio_test_int_is_close(drive_speed, 200, 5));
    tt_want(pbio_test_int_is_close(turn_angle, turn_angle_start, 5));
    tt_want(pbio_test_int_is_close(turn_rate, 0, 5));

    // Test driving/turning forever, maintaining the speed we are already on.
    tt_uint_op(pbio_drivebase_drive_forever(db, 200, 90), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle, &turn_rate), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(drive_distance, 2000, 20));
    tt_want(pbio_test_int_is_close(drive_speed, 200, 5));
    tt_want(pbio_test_int_is_close(turn_angle, turn_angle_start, 5));
    tt_want(pbio_test_int_is_close(turn_rate, 0, 5));

    // After a while, the target speed/rate should be reached.
    pbio_test_sleep_ms(&timer, 2000);
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle, &turn_rate), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_drivebase_is_done(db));
    tt_want(pbio_test_int_is_close(drive_speed, 200, 5));
    tt_want(pbio_test_int_is_close(turn_rate, 90, 5));
    tt_uint_op(pbio_drivebase_is_stalled(db, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(!stalled);

    // Test a small curve.
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle_start, &turn_rate), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_drivebase_drive_curve(db, 10, 360, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_drivebase_is_done(db));
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle, &turn_rate), ==, PBIO_SUCCESS);
    tt_want(pbio_test_int_is_close(turn_angle, turn_angle_start + 360, 5));
    tt_uint_op(pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_HOLD), ==, PBIO_SUCCESS);

    // Stopping a single servo should stop both servos and the drivebase.
    pbio_dcmotor_get_state(srv_left->dcmotor, &actuation, &voltage);
    tt_uint_op(actuation, ==, PBIO_DCMOTOR_ACTUATION_VOLTAGE);
    pbio_dcmotor_get_state(srv_right->dcmotor, &actuation, &voltage);
    tt_uint_op(actuation, ==, PBIO_DCMOTOR_ACTUATION_VOLTAGE);
    tt_uint_op(pbio_servo_stop(srv_left, PBIO_CONTROL_ON_COMPLETION_COAST), ==, PBIO_SUCCESS);
    pbio_dcmotor_get_state(srv_left->dcmotor, &actuation, &voltage);
    tt_uint_op(actuation, ==, PBIO_DCMOTOR_ACTUATION_COAST);
    pbio_dcmotor_get_state(srv_right->dcmotor, &actuation, &voltage);
    tt_uint_op(actuation, ==, PBIO_DCMOTOR_ACTUATION_COAST);

    // Closing any motor should make drivebase operations invalid.
    tt_uint_op(pbio_dcmotor_close(srv_left->dcmotor), ==, PBIO_SUCCESS);
    pbio_test_sleep_ms(&timer, 100);
    tt_uint_op(pbio_drivebase_is_stalled(db, &stalled, &stall_duration), ==, PBIO_ERROR_INVALID_OP);

end:

    PT_END(pt);
}

/**
 * Creates a drivebase pair where one of the motors (C) is physically stalled at
 * about +/- 150 degrees. Motor F can spin freely.
 */
static PT_THREAD(test_drivebase_stalling(struct pt *pt)) {

    static struct timer timer;

    static pbio_servo_t *srv_free;
    static pbio_servo_t *srv_blocked;
    static pbio_drivebase_t *db;
    static pbio_port_t *port;

    static int32_t drive_distance;
    static int32_t drive_speed;
    static int32_t turn_angle_start;
    static int32_t turn_rate;

    static bool stalled;
    static uint32_t stall_duration;

    static uint32_t delay;

    PT_BEGIN(pt);

    // Give simulator some time to start reporting data.
    for (delay = 0; delay < 100; delay++) {
        pbio_test_clock_tick(1);
        PT_YIELD(pt);
    }

    // Initialize the servos.

    lego_device_type_id_t id = LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR;
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_F, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv_free), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv_free, id, PBIO_DIRECTION_COUNTERCLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_port(PBIO_PORT_ID_C, &port), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_port_get_servo(port, &id, &srv_blocked), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(srv_blocked, id, PBIO_DIRECTION_CLOCKWISE, 1000, true, 0), ==, PBIO_SUCCESS);

    // Set up the drivebase.
    tt_uint_op(pbio_drivebase_get_drivebase(&db, srv_free, srv_blocked, 56000, 112000), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_drivebase_get_state_user(db, &drive_distance, &drive_speed, &turn_angle_start, &turn_rate), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_drivebase_is_stalled(db, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(!stalled);

    // Try to drive straight. Should get stalled.
    tt_uint_op(pbio_drivebase_drive_straight(db, 1000, PBIO_CONTROL_ON_COMPLETION_COAST_SMART), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_drivebase_is_stalled(db, &stalled, &stall_duration) == PBIO_SUCCESS && stalled);

    // Stop. Now we should not be stalled any more.
    tt_uint_op(pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_COAST), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_drivebase_is_stalled(db, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(!stalled);

    // Run the individual servos back to the start position.
    tt_uint_op(pbio_servo_run_target(srv_free, 500, 0, PBIO_CONTROL_ON_COMPLETION_COAST), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_run_target(srv_blocked, 500, 0, PBIO_CONTROL_ON_COMPLETION_COAST), ==, PBIO_SUCCESS);
    pbio_test_sleep_until(pbio_control_is_done(&srv_free->control) && pbio_control_is_done(&srv_blocked->control));

    // Stall the individual servos.
    tt_uint_op(pbio_servo_run_target(srv_free, 500, 360, PBIO_CONTROL_ON_COMPLETION_COAST), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_run_target(srv_blocked, 500, 360, PBIO_CONTROL_ON_COMPLETION_COAST), ==, PBIO_SUCCESS);
    pbio_test_sleep_ms(&timer, 2000);

    // The blocked motor should be stalled, the free motor not stalled, and the drivebase stalled.
    tt_uint_op(pbio_servo_is_stalled(srv_blocked, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(stalled);
    tt_uint_op(pbio_servo_is_stalled(srv_free, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(!stalled);
    tt_uint_op(pbio_drivebase_is_stalled(db, &stalled, &stall_duration), ==, PBIO_SUCCESS);
    tt_want(stalled);

end:

    PT_END(pt);
}

struct testcase_t pbio_drivebase_tests[] = {
    PBIO_PT_THREAD_TEST_WITH_PBIO(test_drivebase_basics),
    PBIO_PT_THREAD_TEST_WITH_PBIO(test_drivebase_stalling),
    END_OF_TESTCASES
};
