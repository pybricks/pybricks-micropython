// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <contiki.h>

#include "../core.h"
#include <pbdrv/clock.h>
#include <pbdrv/ioport.h>
#include <pbdrv/motor_driver.h>

#include <pbio/battery.h>
#include <pbio/observer.h>

#include "motor_driver_virtual_simulation.h"

typedef struct _pbio_simulation_model_t {
    double d_angle_d_speed;
    double d_speed_d_speed;
    double d_current_d_speed;
    double d_angle_d_current;
    double d_speed_d_current;
    double d_current_d_current;
    double d_angle_d_voltage;
    double d_speed_d_voltage;
    double d_current_d_voltage;
    double d_angle_d_torque;
    double d_speed_d_torque;
    double d_current_d_torque;
    double torque_friction;
} pbio_simulation_model_t;

struct _pbdrv_motor_driver_dev_t {
    double angle;
    double current;
    double speed;
    double voltage;
    double torque;
    const pbio_simulation_model_t *model;
    const pbdrv_motor_driver_virtual_simulation_platform_data_t *pdata;
};

static const pbio_simulation_model_t model_technic_m_angular = {
    .d_angle_d_speed = 0.0009981527613056019,
    .d_speed_d_speed = 0.994653578576391,
    .d_current_d_speed = -0.0021977502690683696,
    .d_angle_d_current = 0.001957577848006867,
    .d_speed_d_current = 3.640640918794361,
    .d_current_d_current = 0.6348769647439378,
    .d_angle_d_voltage = 0.0002818172865566039,
    .d_speed_d_voltage = 0.815657436669528,
    .d_current_d_voltage = 0.335291816502189,
    .d_angle_d_torque = -9.498678309037282e-05,
    .d_speed_d_torque = -0.18980175337809,
    .d_current_d_torque = 0.0002247101788128779,
    .torque_friction = 21413.268,
};

static pbdrv_motor_driver_dev_t motor_driver_devs[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV];

pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    if (id >= PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *driver = &motor_driver_devs[id];

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {
    driver->voltage = 0.0;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    driver->voltage = pbio_battery_get_voltage_from_duty(duty_cycle);
    return PBIO_SUCCESS;
}

static pid_t data_parser_pid;
static FILE *data_parser_in;

PROCESS(pbdrv_motor_driver_virtual_simulation_process, "pbdrv_motor_driver_virtual_simulation");

PROCESS_THREAD(pbdrv_motor_driver_virtual_simulation_process, ev, data) {
    static struct etimer tick_timer;
    static struct timer frame_timer;

    static uint32_t dev_index;
    static pbdrv_motor_driver_dev_t *driver;

    PROCESS_BEGIN();

    // Initialize driver from platform data.
    for (dev_index = 0; dev_index < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; dev_index++) {
        // Get driver and platform data.
        driver = &motor_driver_devs[dev_index];
        driver->pdata = &pbdrv_motor_driver_virtual_simulation_platform_data[dev_index];
        driver->angle = driver->pdata->initial_angle;
        driver->speed = driver->pdata->initial_speed;
        driver->current = 0;
        driver->torque = 0;
        driver->voltage = 0;

        // Get device ID.
        static pbio_iodev_type_id_t type_id;
        pbio_error_t err = pbdrv_ioport_get_motor_device_type_id(driver->pdata->port_id, &type_id);
        if (err != PBIO_SUCCESS && err != PBIO_ERROR_NO_DEV) {
            PROCESS_EXIT();
        }

        // Select model corresponding to device ID.
        switch (type_id) {
            case PBIO_IODEV_TYPE_ID_SPIKE_S_MOTOR:
                driver->model = &model_technic_m_angular; // TODO
                break;
            case PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR:
                driver->model = &model_technic_m_angular;
                break;
            case PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR:
                driver->model = &model_technic_m_angular; // TODO
                break;
            case PBIO_IODEV_TYPE_ID_NONE:
                driver->model = NULL;
                break;
            default:
                PROCESS_EXIT();
        }
    }

    pbdrv_init_busy_down();

    etimer_set(&tick_timer, 1);
    timer_set(&frame_timer, 40);

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&tick_timer));

        // If data parser pipe is connected, output the motor angles.
        if (data_parser_in && timer_expired(&frame_timer)) {
            timer_reset(&frame_timer);

            // Output motor angles on one line.
            for (dev_index = 0; dev_index < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; dev_index++) {
                driver = &motor_driver_devs[dev_index];
                fprintf(data_parser_in, "%d ", ((int32_t)(driver->angle / 1000)));
            }
            fprintf(data_parser_in, "\r\n");

            // Check that process is still running.
            pid_t p = waitpid(data_parser_pid, NULL, WNOHANG);
            if (p == -1 || p == data_parser_pid) {
                fclose(data_parser_in);
                printf("Process failed or ended.");
                exit(1);
            }
            if (fflush(data_parser_in) == -1) {
                printf("Flush failed.");
                fclose(data_parser_in);
                exit(1);
            }
        }

        for (dev_index = 0; dev_index < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; dev_index++) {
            driver = &motor_driver_devs[dev_index];

            // Skip simulating if there is no model.
            if (!driver->model) {
                continue;
            }

            // Shorthand notation for frequent local references to model.
            const pbio_simulation_model_t *m = driver->model;

            // Modified coulomb friction with transition linear in speed through origin.
            const double limit = 2000;
            double friction;
            if (driver->speed > limit) {
                friction = m->torque_friction;
            } else if (driver->speed < -limit) {
                friction = -m->torque_friction;
            } else {
                friction = m->torque_friction * driver->speed / limit;
            }

            // Stall obstacle torque
            double external_torque = 0;
            if (driver->angle > driver->pdata->endstop_angle_positive) {
                external_torque = (driver->angle - driver->pdata->endstop_angle_positive) * 500 + driver->speed * 5;
            } else if (driver->angle < driver->pdata->endstop_angle_negative) {
                external_torque = (driver->angle - driver->pdata->endstop_angle_negative) * 500 + driver->speed * 5;
            }

            double voltage = driver->voltage;
            double torque = friction + external_torque;

            // Get next state based on current state and input: x(k+1) = Ax(k) + Bu(k)
            double angle_next = driver->angle +
                driver->speed * m->d_angle_d_speed +
                driver->current * m->d_angle_d_current +
                voltage * m->d_angle_d_voltage +
                torque * m->d_angle_d_torque;
            double speed_next = 0 +
                driver->speed * m->d_speed_d_speed +
                driver->current * m->d_speed_d_current +
                voltage * m->d_speed_d_voltage +
                torque * m->d_speed_d_torque;
            double current_next = 0 +
                driver->speed * m->d_current_d_speed +
                driver->current * m->d_current_d_current +
                voltage * m->d_current_d_voltage +
                torque * m->d_current_d_torque;

            // Save new state.
            driver->angle = angle_next;
            driver->speed = speed_next;
            driver->current = current_next;
        }

        etimer_reset(&tick_timer);
    }

    PROCESS_END();
}

// Optionally starts script that receives motor angles through a pipe
// for visualization and debugging purposes.
static void pbdrv_motor_driver_virtual_simulation_prepare_parser(void) {

    const char *data_parser_cmd = getenv("PBIO_TEST_DATA_PARSER");

    // Skip if no data parser is given.
    if (!data_parser_cmd) {
        return;
    }

    // Create the pipe to parser script.
    int data_parser_stdin[2];
    if (pipe(data_parser_stdin) == -1) {
        printf("pipe(data_parser_in) failed\n");
        return;
    }

    // For the process to run the parser in parallel.
    data_parser_pid = fork();
    if (data_parser_pid == -1) {
        printf("fork() failed");
        return;
    }

    // The child process executes the data parser.
    if (data_parser_pid == 0) {
        dup2(data_parser_stdin[0], STDIN_FILENO);
        close(data_parser_stdin[1]);
        char *args[] = {NULL, NULL};
        if (execvp(data_parser_cmd, args) == -1) {
            printf("Failed to start data parser.");
        }
        exit(EXIT_FAILURE);
    }

    // Get file streams for pipes to data parser.
    close(data_parser_stdin[0]);
    data_parser_in = fdopen(data_parser_stdin[1], "w");
    if (data_parser_in == NULL) {
        printf("fdopen(data_parser_stdin[1], \"w\") failed");
        exit(1);
    }
}

static void pbdrv_motor_driver_start_simulation(void) {
    pbdrv_init_busy_up();
    process_start(&pbdrv_motor_driver_virtual_simulation_process);
}

void pbdrv_motor_driver_init(void) {
    pbdrv_motor_driver_virtual_simulation_prepare_parser();
    #if PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION_AUTO_START
    pbdrv_motor_driver_start_simulation();
    #endif
}

#if !PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION_AUTO_START
void pbdrv_motor_driver_init_manual(void) {
    // Motor tests can start the simulation as needed.
    pbdrv_motor_driver_start_simulation();
}
#endif // !PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION_AUTO_START

void pbdrv_motor_driver_virtual_simulation_get_angle(pbdrv_motor_driver_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {
    *rotations = (int64_t)dev->angle / 360000;
    *millidegrees = (int64_t)dev->angle - *rotations * 360000;
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION
