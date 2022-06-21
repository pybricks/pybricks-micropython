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
#include <pbio/servo.h>
#include <test-pbio.h>

#include "../src/processes.h"

// wait an additional 1 second before ending test
#define TEST_END_COUNT 1000 // clock ticks

// Motor driver implementation

typedef enum {
    H_BRIDGE_OUTPUT_LL,
    H_BRIDGE_OUTPUT_LH,
    H_BRIDGE_OUTPUT_HL,
    H_BRIDGE_OUTPUT_HH,
} h_bridge_output_t;

static struct _pbdrv_motor_driver_dev_t {
    uint16_t duty_cycle;
    h_bridge_output_t output;
} test_motor_drivers[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV];

pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    if (id >= PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *driver = &test_motor_drivers[id];

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {
    driver->output = H_BRIDGE_OUTPUT_LL;
    driver->duty_cycle = 0;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    tt_want_int_op(duty_cycle, >=, -PBDRV_MOTOR_DRIVER_MAX_DUTY);
    tt_want_int_op(duty_cycle, <=, PBDRV_MOTOR_DRIVER_MAX_DUTY);

    if (duty_cycle > 0) {
        driver->output = H_BRIDGE_OUTPUT_LH;
    } else if (duty_cycle < 0) {
        driver->output = H_BRIDGE_OUTPUT_HL;
    } else {
        driver->output = H_BRIDGE_OUTPUT_HH;
    }

    driver->duty_cycle = abs(duty_cycle);

    return PBIO_SUCCESS;
}

void pbdrv_motor_driver_init(void) {
}

// I/O port driver implementation

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t id, pbio_iodev_type_id_t *type_id) {
    const char *motor_id = getenv("PBIO_TEST_MOTOR_TYPE");

    if (motor_id) {
        *type_id = atoi(motor_id);
    } else {
        // default value if environment variable is not set
        *type_id = PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR;
    }


    return PBIO_SUCCESS;
}


// Tests

/**
 * Common test for pbio_servo_run_* functions.
 * @param [in]  pt      The Contiki proto-thread
 * @param [in]  name    The name of the test (i.e. __func__)
 * @param [in]  func    Callback for invoking pbio_servo_run_*
 */
static PT_THREAD(test_servo_run_func(struct pt *pt, const char *name, pbio_error_t (*func)(pbio_servo_t *servo))) {
    static pbdrv_motor_driver_dev_t *driver;
    static pbio_servo_t *servo;
    static int32_t *log_buf = NULL;
    static FILE *log_file;
    static uint32_t control_done_count;

    PT_BEGIN(pt);

    tt_uint_op(pbdrv_motor_driver_get_dev(0, &driver), ==, PBIO_SUCCESS);

    process_start(&pbio_motor_process);
    tt_want(process_is_running(&pbio_motor_process));

    tt_uint_op(pbio_servo_get_servo(PBIO_PORT_ID_A, &servo), ==, PBIO_SUCCESS);
    tt_uint_op(pbio_servo_setup(servo, PBIO_DIRECTION_CLOCKWISE, F16C(1, 0), true), ==, PBIO_SUCCESS);

    // only logging one row since we read it after every iteration
    log_buf = malloc(sizeof(*log_buf) * pbio_logger_cols(&servo->control.log));
    if (log_buf == NULL) {
        tt_abort_perror("failed to allocate log_buf");
    }

    pbio_logger_start(&servo->control.log, log_buf, 1, 1);

    tt_uint_op(func(servo), ==, PBIO_SUCCESS);

    char csv_file[FILENAME_MAX];
    sprintf(csv_file, "%s.csv", name);
    log_file = fopen(csv_file, "w");
    if (log_file == NULL) {
        tt_abort_perror("failed to open .csv file");
    }

    fprintf(log_file, "clock time,");
    fprintf(log_file, "h-bridge,");
    fprintf(log_file, "duty cycle,");
    // servo logger columns
    fprintf(log_file, "timestamp,");
    fprintf(log_file, "time since start,");
    fprintf(log_file, "count_now,");
    fprintf(log_file, "rate_now,");
    fprintf(log_file, "actuation,");
    fprintf(log_file, "control,");
    fprintf(log_file, "count_ref,");
    fprintf(log_file, "rate_ref,");
    fprintf(log_file, "err,");
    fprintf(log_file, "err_integral,");
    fprintf(log_file, "\n");

    static pid_t motor_sim_pid;
    static FILE *motor_sim_in;
    static FILE *motor_sim_out;

    // an optional motor simulator program can be passed via environment variable
    const char *motor_sim_cmd = getenv("PBIO_TEST_MOTOR_SIMULATOR");
    if (motor_sim_cmd) {
        int motor_sim_stdin[2];
        int motor_sim_stdout[2];

        if (pipe(motor_sim_stdin) == -1) {
            tt_abort_perror("pipe(motor_sim_in) failed");
        }
        if (pipe(motor_sim_stdout) == -1) {
            tt_abort_perror("pipe(motor_sim_out) failed");
        }

        motor_sim_pid = fork();
        if (motor_sim_pid == -1) {
            tt_abort_perror("fork() failed");
        }
        if (motor_sim_pid == 0) {
            // child process

            dup2(motor_sim_stdin[0], STDIN_FILENO);
            close(motor_sim_stdin[1]);
            close(motor_sim_stdout[0]);
            dup2(motor_sim_stdout[1], STDOUT_FILENO);

            // split PBIO_TEST_MOTOR_SIMULATOR on spaces to get command line args
            char *cmd = strtok(strdup(motor_sim_cmd), " ");
            char **args = NULL;
            int n_args = 1;
            char *arg;
            while ((arg = strtok(NULL, " ")) != NULL) {
                args = realloc(args, sizeof(*args) * (n_args + 1));
                if (args == NULL) {
                    perror("realloc failed");
                    exit(EXIT_FAILURE);
                }
                args[n_args++] = arg;
            }
            args[0] = cmd;
            args[n_args] = NULL;

            if (execvp(cmd, args) == -1) {
                perror("failed to start PBIO_TEST_MOTOR_SIMULATOR");
            }
            exit(EXIT_FAILURE);
        }

        // get file streams for pipes to child process
        close(motor_sim_stdin[0]);
        close(motor_sim_stdout[1]);
        motor_sim_in = fdopen(motor_sim_stdin[1], "w");
        if (motor_sim_in == NULL) {
            tt_abort_perror("fdopen(motor_sim_stdin[1], \"w\") failed");
        }
        motor_sim_out = fdopen(motor_sim_stdout[0], "r");
        if (motor_sim_out == NULL) {
            tt_abort_perror("fdopen(motor_sim_stdout[0], \"r\") failed");
        }
    }

    for (;;) {
        if (pbio_servo_update_loop_is_running(servo)) {
            // This is the expected exit point for a successful test. The manuever
            // has completed. We wait some extra time to log the motor state after
            // the completion before ending the test.
            if (pbio_control_is_done(&servo->control)) {
                if (++control_done_count >= TEST_END_COUNT) {
                    break;
                }
            }

            // if an external motor simulator program was provided, we can use
            // it to get a more accurate test
            if (motor_sim_pid) {
                // write current output state to the motor simulator
                fprintf(motor_sim_in, "%d ", driver->output);
                fprintf(motor_sim_in, "%d ", driver->duty_cycle);
                fprintf(motor_sim_in, "\n");

                // need to check if child process is still running, otherwise
                // fflush can cause a crash instead of returning an error
                pid_t p = waitpid(motor_sim_pid, NULL, WNOHANG);
                if (p == -1) {
                    tt_abort_perror("waitpid failed");
                }
                if (p == motor_sim_pid) {
                    tt_abort_msg("motor simulator exited early");
                }

                if (fflush(motor_sim_in) == -1) {
                    tt_abort_perror("fflush failed");
                }

                // causes test to abort with SIGALRM on timeout during getline,
                // otherwise getline could block forever
                alarm(1);

                // read new input state from the motor simulator
                char *line = NULL;
                size_t n = 0;
                if (getline(&line, &n, motor_sim_out) == -1) {
                    tt_abort_perror("getline failed");
                }

                alarm(0);

                int position, speed;
                int ret = sscanf(line, "%d %d\n", &position, &speed);
                if (ret == EOF) {
                    tt_abort_perror("scanf failed");
                }
                if (ret < 2) {
                    tt_abort_printf(("bad data from motor simulator: %s", line));
                }
                free(line);

                // apply new input state from the motor simulator
                pbio_test_counter_set_angle(position / 360, position % 360);
            } else {
                // allow test to pass even if there is no motor simulator given
                int32_t stall_duration;
                if (pbio_control_is_stalled(&servo->control, &stall_duration)) {
                    break;
                }
            }

            // write current state to log file
            if (pbio_logger_rows(&servo->control.log)) {
                fprintf(log_file, "%d,", pbdrv_clock_get_ms());
                fprintf(log_file, "%d,", driver->output);
                fprintf(log_file, "%d,", driver->duty_cycle);

                for (int i = 0; i < pbio_logger_cols(&servo->control.log); i++) {
                    fprintf(log_file, "%d,", log_buf[i]);
                }

                fprintf(log_file, "\n");
                servo->control.log.sampled = 0;
            }

            pbio_test_clock_tick(1);
            PT_YIELD(pt);
            continue;
        }
        break;
    }

end:
    if (motor_sim_pid) {
        // allow motor simulator program to exit cleanly
        kill(motor_sim_pid, SIGTERM);
        waitpid(motor_sim_pid, NULL, 0);
    }
    if (motor_sim_out) {
        fclose(motor_sim_out);
    }
    if (motor_sim_in) {
        fclose(motor_sim_in);
    }
    if (log_file) {
        fclose(log_file);
    }
    if (log_buf) {
        free(log_buf);
    }

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

struct testcase_t pbio_motor_tests[] = {
    PBIO_PT_THREAD_TEST(test_servo_run_angle),
    PBIO_PT_THREAD_TEST(test_servo_run_time),
    END_OF_TESTCASES
};
