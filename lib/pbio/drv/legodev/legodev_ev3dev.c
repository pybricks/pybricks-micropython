// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LEGODEV_EV3DEV

#include <stdbool.h>

#include <pbdrv/clock.h>
#include <pbdrv/counter.h>
#include <pbdrv/legodev.h>

#include <pbio/util.h>

#include <ev3dev_stretch/lego_motor.h>
#include <ev3dev_stretch/lego_sensor.h>
#include <ev3dev_stretch/nxtcolor.h>

#include "legodev_ev3dev.h"

typedef struct {
    const pbdrv_legodev_ev3dev_sensor_platform_data_t *pdata;
    lego_sensor_t *ev3dev_sensor;
    uint32_t mode_switch_time;
    pbdrv_legodev_info_t info;
    // The color sensor does not have a bin_data buffer, so buffer here.
    int32_t color_sensor_rgba[4];
} ev3dev_sensor_t;

static ev3dev_sensor_t ev3dev_sensor_devs[PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_SENSOR];

struct _pbdrv_legodev_dev_t {
    bool is_motor;
    union {
        // Motor devices have only constant platform data.
        const pbdrv_legodev_ev3dev_motor_platform_data_t *motor;
        // Sensor devices have both a state and constant platform data.
        ev3dev_sensor_t *sensor;
    };
};

static pbdrv_legodev_dev_t devs[PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_MOTOR + PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_SENSOR];

void pbdrv_legodev_init(void) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_MOTOR; i++) {
        pbdrv_legodev_dev_t *legodev = &devs[i];
        legodev->is_motor = true;
        legodev->motor = &pbdrv_legodev_ev3dev_motor_platform_data[i];
    }

    for (uint8_t i = 0; i < PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_SENSOR; i++) {
        pbdrv_legodev_dev_t *legodev = &devs[i + PBDRV_CONFIG_LEGODEV_EV3DEV_NUM_MOTOR];
        legodev->is_motor = false;
        legodev->sensor = &ev3dev_sensor_devs[i];
        legodev->sensor->pdata = &pbdrv_legodev_ev3dev_sensor_platform_data[i];
        legodev->sensor->ev3dev_sensor = NULL;
        legodev->sensor->mode_switch_time = pbdrv_clock_get_ms();
    }
}

pbio_error_t pbdrv_legodev_get_motor_index(pbdrv_legodev_dev_t *legodev, uint8_t *index) {
    if (!legodev->is_motor) {
        return PBIO_ERROR_NO_DEV;
    }
    *index = legodev->motor->motor_driver_index;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    if (!legodev->is_motor) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }
    pbdrv_counter_dev_t *counter;
    pbio_error_t err = pbdrv_counter_get_dev(legodev->motor->motor_driver_index, &counter);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return pbdrv_counter_get_angle(counter, &angle->rotations, &angle->millidegrees);
}

pbio_error_t pbdrv_legodev_get_abs_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

bool type_id_matches(pbdrv_legodev_type_id_t *type, pbdrv_legodev_type_id_t actual_type) {
    return false;
}

pbio_error_t pbdrv_legodev_get_device(pbio_port_id_t port_id, pbdrv_legodev_type_id_t *type_id, pbdrv_legodev_dev_t **legodev) {
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(devs); i++) {
        pbdrv_legodev_dev_t *candidate = &devs[i];

        // Try to get a motor.
        if (candidate->is_motor && candidate->motor->port_id == port_id) {
            pbio_error_t err = ev3dev_motor_setup(port_id, *type_id != PBDRV_LEGODEV_TYPE_ID_ANY_DC_MOTOR);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            *legodev = candidate;
            return ev3dev_motor_get_id(port_id, type_id);
        }

        // Try to get a sensor.
        if (!candidate->is_motor && candidate->sensor->pdata->port_id == port_id) {
            *legodev = candidate;
            pbio_error_t err = lego_sensor_get(&candidate->sensor->ev3dev_sensor, port_id, *type_id);
            if (err != PBIO_SUCCESS) {
                return err;
            }

            // For special sensor classes we are done. No need to read mode.
            if (*type_id == PBDRV_LEGODEV_TYPE_ID_CUSTOM_I2C ||
                *type_id == PBDRV_LEGODEV_TYPE_ID_CUSTOM_UART ||
                *type_id == PBDRV_LEGODEV_TYPE_ID_NXT_COLOR_SENSOR) {
                return PBIO_SUCCESS;
            }
            // Get mode
            pbdrv_legodev_info_t *info = &candidate->sensor->info;
            err = lego_sensor_get_mode(candidate->sensor->ev3dev_sensor, &info->mode);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            // Get corresponding mode info
            pbdrv_legodev_mode_info_t *mode_info = &candidate->sensor->info.mode_info[info->mode];
            return lego_sensor_get_info(candidate->sensor->ev3dev_sensor, &mode_info->num_values, (lego_sensor_data_type_t *)&mode_info->data_type);
        }
    }
    return PBIO_ERROR_NO_DEV;
}

bool pbdrv_legodev_needs_permanent_power(pbdrv_legodev_dev_t *legodev) {
    return false;
}

pbio_error_t pbdrv_legodev_get_info(pbdrv_legodev_dev_t *legodev, pbdrv_legodev_info_t **info) {
    *info = &legodev->sensor->info;
    return PBIO_SUCCESS;
}

// Get the required mode switch time delay for a given sensor type and/or mode
static uint32_t get_mode_switch_delay(pbdrv_legodev_type_id_t id, uint8_t mode) {
    switch (id) {
        case PBDRV_LEGODEV_TYPE_ID_EV3_COLOR_SENSOR:
            return 30;
        case PBDRV_LEGODEV_TYPE_ID_EV3_IR_SENSOR:
            return 1100;
        case PBDRV_LEGODEV_TYPE_ID_NXT_LIGHT_SENSOR:
            return 20;
        case PBDRV_LEGODEV_TYPE_ID_NXT_SOUND_SENSOR:
            return 300;
        case PBDRV_LEGODEV_TYPE_ID_NXT_ENERGY_METER:
            return 200;
        // Default delay for other sensors and modes:
        default:
            return 0;
    }
}

pbio_error_t pbdrv_legodev_is_ready(pbdrv_legodev_dev_t *legodev) {
    if (legodev->is_motor) {
        return PBIO_SUCCESS;
    }

    if (!legodev->sensor->ev3dev_sensor) {
        return PBIO_ERROR_NO_DEV;
    }

    // Some device/mode pairs require time to discard stale data
    uint32_t delay = get_mode_switch_delay(legodev->sensor->info.type_id, legodev->sensor->info.mode);
    if (pbdrv_clock_get_ms() - legodev->sensor->mode_switch_time < delay) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_set_mode(pbdrv_legodev_dev_t *legodev, uint8_t mode) {
    if (legodev->is_motor) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    if (!legodev->sensor->ev3dev_sensor) {
        return PBIO_ERROR_NO_DEV;
    }

    // Some device/mode pairs always require setting the mode.
    pbdrv_legodev_info_t *info = &legodev->sensor->info;
    bool mode_set_required = info->type_id == PBDRV_LEGODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR &&
        mode >= PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM;

    // If mode already set and no change required, we are done.
    if (info->mode == mode && !mode_set_required) {
        return PBIO_SUCCESS;
    }

    // Set new mode.
    pbio_error_t err = lego_sensor_set_mode(legodev->sensor->ev3dev_sensor, mode);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    info->mode = mode;
    legodev->sensor->mode_switch_time = pbdrv_clock_get_ms();

    // Get matching mode info.
    pbdrv_legodev_mode_info_t *mode_info = &legodev->sensor->info.mode_info[info->mode];
    return lego_sensor_get_info(legodev->sensor->ev3dev_sensor, &mode_info->num_values, (lego_sensor_data_type_t *)&mode_info->data_type);
}

pbio_error_t pbdrv_legodev_set_mode_with_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, const void *data, uint8_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_legodev_get_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, void **data) {
    if (legodev->is_motor) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    if (!legodev->sensor->ev3dev_sensor) {
        return PBIO_ERROR_NO_DEV;
    }

    // The NXT Color Sensor has a custom routine not part of ev3dev.
    if (legodev->sensor->info.type_id == PBDRV_LEGODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        *data = legodev->sensor->color_sensor_rgba;
        return nxtcolor_get_values_at_mode(legodev->sensor->pdata->port_id, legodev->sensor->info.mode, legodev->sensor->color_sensor_rgba);
    }

    // Get data from ev3dev.
    return lego_sensor_get_bin_data(legodev->sensor->ev3dev_sensor, (uint8_t **)data);
}

#endif // PBDRV_CONFIG_LEGODEV_EV3DEV
