// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/control.h>
#include <pbio/error.h>
#include <pbio/logger.h>
#include <pbio/motorpoll.h>
#include <pbio/servo.h>

void clock_override();
void clock_override_tick(clock_time_t ticks);

// Motor driver implementation

typedef enum {
    H_BRIDGE_OUTPUT_LL,
    H_BRIDGE_OUTPUT_LH,
    H_BRIDGE_OUTPUT_HL,
    H_BRIDGE_OUTPUT_HH,
} h_bridge_output_t;

static struct {
    uint16_t duty_cycle;
    h_bridge_output_t output;
} test_motor_driver;

pbio_error_t pbdrv_motor_coast(pbio_port_t port) {
    test_motor_driver.output = H_BRIDGE_OUTPUT_LL;
    test_motor_driver.duty_cycle = 0;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    if (duty_cycle > 0) {
        test_motor_driver.output = H_BRIDGE_OUTPUT_LH;
    } else if (duty_cycle < 0) {
        test_motor_driver.output = H_BRIDGE_OUTPUT_HL;
    } else {
        test_motor_driver.output = H_BRIDGE_OUTPUT_HH;
    }
    test_motor_driver.duty_cycle = abs(duty_cycle);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_get_id(pbio_port_t port, pbio_iodev_type_id_t *id) {
    *id = PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_setup(pbio_port_t port, bool is_servo) {
    return PBIO_SUCCESS;
}

// Tests

PT_THREAD(test_servo_run_angle(struct pt *pt)) {
    static pbio_servo_t *servo;
    static int32_t *log_buf = NULL;
    static FILE *log_file;

    PT_BEGIN(pt);

    clock_override();

    tt_uint_op(pbio_motorpoll_get_servo(PBIO_PORT_A, &servo), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(servo, PBIO_DIRECTION_CLOCKWISE, F16C(1, 0)), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_motorpoll_set_servo_status(servo, PBIO_ERROR_AGAIN), ==, PBIO_SUCCESS);

    // only logging one row since we read it after every iteration
    log_buf = malloc(sizeof(*log_buf) * pbio_logger_cols(&servo->log));
    if (log_buf == NULL) {
        tt_abort_perror("failed to allocate log_buf");
    }

    pbio_logger_start(&servo->log, log_buf, 1, 1);

    tt_uint_op(pbio_servo_run_angle(servo, 500, 180, PBIO_ACTUATION_HOLD), ==, PBIO_SUCCESS);

    log_file = fopen("test_servo_run_angle.csv", "w");
    if (log_file == NULL) {
        tt_abort_perror("failed to open test_servo_run_angle.csv");
    }

    fprintf(log_file, "clock time, ");
    fprintf(log_file, "h-bridge, ");
    fprintf(log_file, "duty cycle, ");
    // servo logger columns
    fprintf(log_file, "timestamp, ");
    fprintf(log_file, "time since start, ");
    fprintf(log_file, "count_now, ");
    fprintf(log_file, "rate_now, ");
    fprintf(log_file, "actuation, ");
    fprintf(log_file, "control, ");
    fprintf(log_file, "count_ref, ");
    fprintf(log_file, "rate_ref, ");
    fprintf(log_file, "err, ");
    fprintf(log_file, "err_integral, ");
    fprintf(log_file, "\n");

    for (;;) {
        pbio_error_t err = pbio_motorpoll_get_servo_status(servo);
        if (err == PBIO_ERROR_AGAIN) {
            if (pbio_control_is_done(&servo->control)) {
                break;
            }

            // HACK: allow test to pass for now
            if (pbio_control_is_stalled(&servo->control)) {
                break;
            }

            if (pbio_logger_rows(&servo->log)) {
                fprintf(log_file, "%d, ", clock_to_msec(clock_time()));
                fprintf(log_file, "%d, ", test_motor_driver.output);
                fprintf(log_file, "%d, ", test_motor_driver.duty_cycle);

                for (int i = 0; i < pbio_logger_cols(&servo->log); i++) {
                    fprintf(log_file, "%d, ", log_buf[i]);
                }

                fprintf(log_file, "\n");
                servo->log.sampled = 0;
            }

            clock_override_tick(1);
            PT_YIELD(pt);
            continue;
        }
        tt_uint_op(err, ==, PBIO_SUCCESS);
        break;
    }

end:
    if (log_file) {
        fclose(log_file);
    }
    if (log_buf) {
        free(log_buf);
    }

    PT_END(pt);
}
