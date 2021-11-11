// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/battery.h>
#include <pbio/control.h>
#include <pbio/drivebase.h>
#include <pbio/motor_process.h>
#include <pbio/servo.h>

#include <contiki.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

PROCESS(pbio_motor_process, "servo");

static pbio_error_t status = PBIO_SUCCESS;
static pbio_servo_t servos[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];
static pbio_drivebase_t drivebase;

pbio_error_t pbio_motor_process_get_status(void) {
    return status;
}

pbio_error_t pbio_motor_process_get_drivebase(pbio_drivebase_t **db) {
    *db = &drivebase;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_process_get_servo(pbio_port_id_t port, pbio_servo_t **srv) {

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    *srv = &servos[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    (*srv)->port = port;
    return PBIO_SUCCESS;
}

void pbio_motor_process_reset(void) {

    // Motors start in success state.
    status = PBIO_SUCCESS;

    pbio_error_t err;

    // Initialize battery voltage.
    err = pbio_battery_init();
    if (err != PBIO_SUCCESS) {
        status = err;
    }

    // Force stop the drivebase
    pbio_drivebase_stop_control(&drivebase);

    // Force stop the servos
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {

        // Run the motor getter at least once
        pbio_servo_t *srv;
        err = pbio_motor_process_get_servo(i + PBDRV_CONFIG_FIRST_MOTOR_PORT, &srv);
        if (err != PBIO_SUCCESS) {
            status = err;
        }

        // Force stop the servo
        pbio_servo_stop_control(srv);
    }
}

PROCESS_THREAD(pbio_motor_process, ev, data) {
    static struct etimer timer;

    static pbio_error_t err;

    PROCESS_BEGIN();

    pbio_motor_process_reset();

    etimer_set(&timer, PBIO_CONTROL_LOOP_TIME_MS);

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        // Update battery voltage.
        err = pbio_battery_update();
        if (err != PBIO_SUCCESS) {
            status = err;
        }

        // Update drivebase
        err = pbio_drivebase_update(&drivebase);
        if (err != PBIO_SUCCESS) {
            status = err;
        }

        // Update servos
        for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
            err = pbio_servo_update(&servos[i]);
            if (err != PBIO_SUCCESS) {
                status = err;
            }
        }

        // Reset timer to wait for next update
        etimer_restart(&timer);
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
