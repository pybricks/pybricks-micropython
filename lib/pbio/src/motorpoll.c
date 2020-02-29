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

    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        if (servo[i].port == port) {
            *srv = &servo[i];
            return PBIO_SUCCESS;
        }
    }
    return PBIO_ERROR_INVALID_PORT;
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

    // Set ports for all servos on init
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        servo[i].port = PBIO_PORT_A + i;
    }

    pbio_error_t err;

    // Force stop the drivebase
    err = pbio_drivebase_stop_force(&drivebase);
    if (err != PBIO_SUCCESS) {
        drivebase_err = err;
    }

    // Force stop the servos
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        err = pbio_servo_stop_force(&servo[i]);
        if (err != PBIO_SUCCESS) {
            servo_err[i] = err;
        }
    }
}

void _pbio_motorpoll_poll(void) {

    pbio_error_t err;

    // Poll servos
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        // Poll servo again if it says so, and save error if encountered
        if (servo_err[i] == PBIO_ERROR_AGAIN) {
            err = pbio_servo_control_update(&servo[i]);
            if (err != PBIO_SUCCESS) {
                servo_err[i] = err;
                // TODO: Raise this error
            }
        }
    }

    // Poll drivebase again if it says so, and save error if encountered
    if (drivebase_err == PBIO_ERROR_AGAIN) {
        err = pbio_drivebase_update(&drivebase);
        if (err != PBIO_SUCCESS) {
            drivebase_err = err;
            // TODO: Raise this error
        }
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
