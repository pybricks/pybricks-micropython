// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION

#include <contiki.h>

#include <pbdrv/clock.h>
#include <pbdrv/counter.h>
#include <pbdrv/motor_driver.h>

#include <pbio/battery.h>
#include <pbio/busy_count.h>
#include <pbio/observer.h>
#include <pbio/port_interface.h>
#include <pbio/util.h>

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

// This motor driver also implements the counter driver.
struct _pbdrv_counter_dev_t {
    pbdrv_motor_driver_dev_t *motor_driver;
};

struct _pbdrv_motor_driver_dev_t {
    double angle;
    double current;
    double speed;
    double voltage;
    double torque;
    const pbio_simulation_model_t *model;
    const pbdrv_motor_driver_virtual_simulation_platform_data_t *pdata;
    pbdrv_counter_dev_t counter;
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

static void simulation_init(void) {

    // This simulation implements the counter and motor driver in one, with
    // a common initialization. We don't know which will initialize first, so
    // both will call this and we need to run it just once.
    static bool has_initialized = false;
    if (has_initialized) {
        return;
    }
    has_initialized = true;

    // Initialize driver from platform data.
    for (uint32_t dev_index = 0; dev_index < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; dev_index++) {
        // Get driver and platform data.
        pbdrv_motor_driver_dev_t *driver = &motor_driver_devs[dev_index];
        driver->pdata = &pbdrv_motor_driver_virtual_simulation_platform_data[dev_index];
        driver->angle = driver->pdata->initial_angle;
        driver->speed = driver->pdata->initial_speed;
        driver->current = 0;
        driver->torque = 0;
        driver->voltage = 0;

        // Select model corresponding to device ID.
        switch (driver->pdata->type_id) {
            case LEGO_DEVICE_TYPE_ID_SPIKE_S_MOTOR:
                driver->model = &model_technic_m_angular; // TODO
                break;
            case LEGO_DEVICE_TYPE_ID_SPIKE_M_MOTOR:
                driver->model = &model_technic_m_angular;
                break;
            case LEGO_DEVICE_TYPE_ID_SPIKE_L_MOTOR:
                driver->model = &model_technic_m_angular; // TODO
                break;
            default:
            case LEGO_DEVICE_TYPE_ID_NONE:
                driver->model = NULL;
                break;
        }
    }
}

pbio_error_t pbdrv_counter_get_dev(uint8_t id, pbdrv_counter_dev_t **dev) {
    if (id >= PBIO_ARRAY_SIZE(motor_driver_devs) || motor_driver_devs[id].pdata->type_id == LEGO_DEVICE_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }
    *dev = &motor_driver_devs[id].counter;
    (*dev)->motor_driver = &motor_driver_devs[id];
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_counter_assert_type(pbdrv_counter_dev_t *dev, lego_device_type_id_t *expected_type_id) {
    lego_device_type_id_t id = dev->motor_driver->pdata->type_id;

    // There is no counter here.
    if (id == LEGO_DEVICE_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }

    // All motors in this simulation driver have encodeders.
    if (*expected_type_id == LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR) {
        *expected_type_id = id;
        return PBIO_SUCCESS;
    }

    // Otherwise require exact match.
    if (*expected_type_id == id) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_NO_DEV;
}

pbio_error_t pbdrv_counter_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {
    *rotations = (int32_t)(dev->motor_driver->angle / 360000);
    *millidegrees = (int32_t)(dev->motor_driver->angle) % 360000;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_counter_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {
    *millidegrees = ((int32_t)dev->motor_driver->angle) % 360000;
    if (*millidegrees > 180000) {
        *millidegrees -= 360000;
    } else if (*millidegrees < -180000) {
        *millidegrees += 360000;
    }
    return PBIO_SUCCESS;
}

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

PROCESS(pbdrv_motor_driver_virtual_simulation_process, "pbdrv_motor_driver_virtual_simulation");

PROCESS_THREAD(pbdrv_motor_driver_virtual_simulation_process, ev, data) {
    static struct etimer tick_timer;
    static struct timer frame_timer;

    static uint32_t dev_index;
    static pbdrv_motor_driver_dev_t *driver;

    PROCESS_BEGIN();

    etimer_set(&tick_timer, 1);
    timer_set(&frame_timer, 40);

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&tick_timer));

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

static bool simulation_enabled = true;

void pbdrv_motor_driver_disable_process(void) {
    simulation_enabled = false;
}

void pbdrv_counter_init(void) {
    simulation_init();
}

void pbdrv_motor_driver_init(void) {
    simulation_init();

    if (simulation_enabled) {
        process_start(&pbdrv_motor_driver_virtual_simulation_process);
    }
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL_SIMULATION
