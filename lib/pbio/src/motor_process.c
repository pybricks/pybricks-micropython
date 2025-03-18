// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include <pbdrv/clock.h>

#include <pbio/battery.h>
#include <pbio/control.h>
#include <pbio/drivebase.h>
#include <pbio/servo.h>

#include <pbio/os.h>

#if PBIO_CONFIG_MOTOR_PROCESS != 0

static pbio_os_process_t pbio_motor_process;

static pbio_error_t pbio_motor_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    ASYNC_BEGIN(state);

    timer.start = pbdrv_clock_get_ms() - PBIO_CONFIG_CONTROL_LOOP_TIME_MS;
    timer.duration = PBIO_CONFIG_CONTROL_LOOP_TIME_MS;

    // Initialize battery voltage.
    pbio_battery_init();

    for (;;) {
        // Update battery voltage.
        pbio_battery_update();

        // Update drivebase
        pbio_drivebase_update_all();

        // Update servos
        pbio_servo_update_all();

        // Increment start time instead waiting from here, making the
        // loop time closer to the target on average.
        timer.start += PBIO_CONFIG_CONTROL_LOOP_TIME_MS;

        // In the rare case that polling was delayed too long, we need to
        // ensure that the next poll is a minimum of 1ms in the future so we
        // don't have 0 time deltas in the control code.
        while (pbio_os_timer_is_expired(&timer)) {
            timer.start++;
        }

        AWAIT_UNTIL(state, pbio_os_timer_is_expired(&timer));
    }

    // Unreachable.
    ASYNC_END(PBIO_ERROR_FAILED);
}

void pbio_motor_process_start(void) {
    pbio_os_process_start(&pbio_motor_process, pbio_motor_process_thread, NULL);
}

#endif // PBIO_CONFIG_MOTOR_PROCESS
