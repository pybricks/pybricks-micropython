// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

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

#include <pbio/control.h>
#include <pbio/error.h>
#include <pbio/logger.h>
#include <pbio/motorpoll.h>
#include <pbio/servo.h>

void clock_override();
void clock_override_tick(clock_time_t ticks);
void pbio_test_counter_set_count(int32_t count);
void pbio_test_counter_set_rate(int32_t rate);

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
    const char *motor_id = getenv("PBIO_TEST_MOTOR_TYPE");
    *id = motor_id == NULL ? PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR : atoi(motor_id);
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
        pbio_error_t err = pbio_motorpoll_get_servo_status(servo);
        if (err == PBIO_ERROR_AGAIN) {
            if (pbio_control_is_done(&servo->control)) {
                break;
            }

            // if an external motor simulator program was provided, we can use
            // it to get a more accurate test
            if (motor_sim_pid) {
                // write current output state to the motor simulator
                fprintf(motor_sim_in, "%d ", test_motor_driver.output);
                fprintf(motor_sim_in, "%d ", test_motor_driver.duty_cycle);
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
                pbio_test_counter_set_count(position);
                pbio_test_counter_set_rate(speed);
            } else {
                // allow test to pass even if there is no motor simulator given
                if (pbio_control_is_stalled(&servo->control)) {
                    break;
                }
            }

            // write current state to log file
            if (pbio_logger_rows(&servo->log)) {
                fprintf(log_file, "%d,", clock_to_msec(clock_time()));
                fprintf(log_file, "%d,", test_motor_driver.output);
                fprintf(log_file, "%d,", test_motor_driver.duty_cycle);

                for (int i = 0; i < pbio_logger_cols(&servo->log); i++) {
                    fprintf(log_file, "%d,", log_buf[i]);
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
