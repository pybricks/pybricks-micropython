// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <pbio/battery.h>
#include <pbio/control.h>
#include <pbio/drivebase.h>
#include <pbio/servo.h>

#include <contiki.h>

#if PBIO_CONFIG_MOTOR_PROCESS != 0

PROCESS(pbio_motor_process, "servo");

PROCESS_THREAD(pbio_motor_process, ev, data) {
    static struct etimer timer;

    PROCESS_BEGIN();

    // Initialize battery voltage.
    pbio_battery_init();

    // Initialize motors in stopped state.
    pbio_dcmotor_stop_all(true);

    etimer_set(&timer, PBIO_CONFIG_CONTROL_LOOP_TIME_MS);

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&timer));

        // Update battery voltage.
        pbio_battery_update();

        // Update drivebase
        pbio_drivebase_update_all();

        // Update servos
        pbio_servo_update_all();

        clock_time_t now = clock_time();

        // If polling was delayed too long, we need to ensure that the next
        // poll is a minimum of 1ms in the future. If we don't, the poll loop
        // will not yield until and the next update will be called with a 0 time
        // diff which causes issues.
        if (now - etimer_start_time(&timer) >= 2 * PBIO_CONFIG_CONTROL_LOOP_TIME_MS) {
            timer.timer.start = now - (PBIO_CONFIG_CONTROL_LOOP_TIME_MS - 1);
        }

        // Reset timer to wait for next update. Using etimer_reset() instead
        // of etimer_restart() makes average update period closer to the expected
        // PBIO_CONFIG_CONTROL_LOOP_TIME_MS when occasional delays occur.
        etimer_reset(&timer);
    }

    PROCESS_END();
}

void pbio_motor_process_start(void) {
    process_start(&pbio_motor_process);
}

#endif // PBIO_CONFIG_MOTOR_PROCESS
