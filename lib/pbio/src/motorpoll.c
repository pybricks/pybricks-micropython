// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 Laurens Valk
// Copyright (c) 2020 LEGO System A/S

#include <pbio/control.h>
#include <pbio/drivebase.h>
#include <pbio/motorpoll.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_servo_t servo[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];
static pbio_error_t servo_err[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_drivebase_t drivebase;
static pbio_error_t drivebase_err;

// Get pointer to servo by port index
pbio_error_t pbio_motorpoll_get_servo(pbio_port_t port, pbio_servo_t **srv) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    // Get pointer to servo object
    *srv = &servo[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    (*srv)->port = port;
    return PBIO_SUCCESS;
}

// Set status of the servo, which tells us whether to poll or not
pbio_error_t pbio_motorpoll_set_servo_status(pbio_servo_t *srv, pbio_error_t err) {
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        if (srv == &servo[i]) {
            servo_err[i] = err;
            return PBIO_SUCCESS;
        }
    }
    return PBIO_ERROR_INVALID_ARG;
}

// Get pointer to drivebase
pbio_error_t pbio_motorpoll_get_drivebase(pbio_drivebase_t **db) {
    // Get pointer to device (at the moment, there is only one drivebase)
    *db = &drivebase;
    return PBIO_SUCCESS;
}

// Set status of the drivebase, which tells us whether to poll or not
pbio_error_t pbio_motorpoll_set_drivebase_status(pbio_drivebase_t *db, pbio_error_t err) {
    if (db != &drivebase) {
        return PBIO_ERROR_INVALID_ARG;
    }
    drivebase_err = err;
    return PBIO_SUCCESS;
}

void _pbio_motorpoll_reset_all(void) {

    // Set control status to passive
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbio_control_stop(&servo[i].control);
        servo[i].claimed = false;
    }
    pbio_control_stop(&drivebase.control_distance);
    pbio_control_stop(&drivebase.control_heading);

    // Physically stop the motors and set status to no device
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbdrv_motor_coast(PBIO_PORT_A + i);
        servo_err[i] = PBIO_ERROR_NO_DEV;
    }
    drivebase_err = PBIO_ERROR_NO_DEV;
}

void _pbio_motorpoll_poll(void) {

    // Poll servos
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        // Poll servo if current status is success
        if (servo_err[i] == PBIO_SUCCESS) {
            servo_err[i] = pbio_servo_control_update(&servo[i]);
        }
    }

    // Poll drivebase if enabled
    if (drivebase_err == PBIO_SUCCESS) {
        drivebase_err = pbio_drivebase_update(&drivebase);
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
