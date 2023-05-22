// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <string.h>

#include <pbdrv/config.h>
#include <pbdrv/ioport.h>
#include <pbdrv/motor_driver.h>
#include <pbio/color.h>
#include <pbio/iodev.h>

#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_pb/pb_error.h>

struct _pb_device_t {
    pbio_iodev_t iodev;
};

static void wait(pbio_error_t (*end)(pbio_iodev_t *), pbio_iodev_t *iodev) {
    pbio_error_t err;
    while ((err = end(iodev)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK
    }
    pb_assert(err);
}


// Get the required mode switch time delay for a given sensor type and/or mode
static uint32_t get_mode_switch_delay(pbio_iodev_type_id_t id, uint8_t mode) {
    switch (id) {
        case PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR:
            return 30;
        case PBIO_IODEV_TYPE_ID_SPIKE_COLOR_SENSOR:
            return 30;
        case PBIO_IODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR:
            return 50;
        // Default delay for other sensors and modes:
        default:
            return 0;
    }
}

static void set_mode(pbio_iodev_t *iodev, uint8_t new_mode) {
    pbio_error_t err;

    if (iodev->mode == new_mode) {
        return;
    }

    while ((err = pbio_iodev_set_mode_begin(iodev, new_mode)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK
    }
    pb_assert(err);
    wait(pbio_iodev_set_mode_end, iodev);

    // Give some time for the mode to take effect and discard stale data
    uint32_t delay = get_mode_switch_delay(iodev->info->type_id, new_mode);
    if (delay > 0) {
        mp_hal_delay_ms(delay);
    }
}

pb_device_t *pb_device_get_device(pbio_port_id_t port, pbio_iodev_type_id_t valid_id) {

    // Get the iodevice
    pbio_iodev_t *iodev;
    pbio_error_t err;

    // Set up device
    while ((err = pbdrv_ioport_get_iodev(port, &iodev)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(50);
    }
    pb_assert(err);

    // Verify the ID or always allow generic LUMP device
    if (iodev->info->type_id != valid_id && valid_id != PBIO_IODEV_TYPE_ID_LUMP_UART) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    return (pb_device_t *)iodev;
}

void pb_device_get_values(pb_device_t *pbdev, uint8_t mode, int32_t *values) {

    pbio_iodev_t *iodev = &pbdev->iodev;

    uint8_t *data;
    uint8_t len;
    pbio_iodev_data_type_t type;

    set_mode(iodev, mode);

    pb_assert(pbio_iodev_get_data(iodev, &data));
    pb_assert(pbio_iodev_get_data_format(iodev, iodev->mode, &len, &type));

    if (len == 0) {
        pb_assert(PBIO_ERROR_IO);
    }

    for (uint8_t i = 0; i < len; i++) {
        switch (type & PBIO_IODEV_DATA_TYPE_MASK) {
            case PBIO_IODEV_DATA_TYPE_INT8:
                values[i] = *((int8_t *)(data + i * 1));
                break;
            case PBIO_IODEV_DATA_TYPE_INT16:
                values[i] = *((int16_t *)(data + i * 2));
                break;
            case PBIO_IODEV_DATA_TYPE_INT32:
                values[i] = *((int32_t *)(data + i * 4));
                break;
            #if MICROPY_PY_BUILTINS_FLOAT
            case PBIO_IODEV_DATA_TYPE_FLOAT:
                *(float *)(values + i) = *((float *)(data + i * 4));
                break;
            #endif
            default:
                pb_assert(PBIO_ERROR_IO);
        }
    }
}

void pb_device_set_values(pb_device_t *pbdev, uint8_t mode, int32_t *values, uint8_t num_values) {

    pbio_iodev_t *iodev = &pbdev->iodev;

    uint8_t data[PBIO_IODEV_MAX_DATA_SIZE];
    uint8_t len;
    pbio_iodev_data_type_t type;

    set_mode(iodev, mode);

    pb_assert(pbio_iodev_get_data_format(iodev, iodev->mode, &len, &type));

    if (len != num_values) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    for (uint8_t i = 0; i < len; i++) {
        switch (type & PBIO_IODEV_DATA_TYPE_MASK) {
            case PBIO_IODEV_DATA_TYPE_INT8:
                *(int8_t *)(data + i) = values[i];
                break;
            case PBIO_IODEV_DATA_TYPE_INT16:
                *(int16_t *)(data + i * 2) = values[i];
                break;
            case PBIO_IODEV_DATA_TYPE_INT32:
                *(int32_t *)(data + i * 4) = values[i];
                break;
            #if MICROPY_PY_BUILTINS_FLOAT
            case PBIO_IODEV_DATA_TYPE_FLOAT:
                *(float *)(data + i * 4) = values[i];
                break;
            #endif
            default:
                pb_assert(PBIO_ERROR_IO);
        }
    }
    pbio_error_t err;
    while ((err = pbio_iodev_set_data_begin(iodev, iodev->mode, data)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK
    }
    pb_assert(err);
    wait(pbio_iodev_set_data_end, iodev);

    // Give some time for the set values to take effect
    uint32_t delay = get_mode_switch_delay(iodev->info->type_id, mode);
    if (delay > 0) {
        mp_hal_delay_ms(delay / 10);
    }
}

void pb_device_set_power_supply(pb_device_t *pbdev, int32_t duty) {
    // Bind user input to percentage
    if (duty < 0) {
        duty = 0;
    } else if (duty > 100) {
        duty = 100;
    }

    // FIXME: this should be a callback function on a port instance rather
    // than poking the motor driver directly. The current implementation
    // is only valid on Powered Up platforms and it assumes that motor driver
    // id corresponds to the port.

    #ifdef PBDRV_CONFIG_FIRST_MOTOR_PORT
    pbdrv_motor_driver_dev_t *motor_driver;
    pb_assert(pbdrv_motor_driver_get_dev(pbdev->iodev.port - PBDRV_CONFIG_FIRST_MOTOR_PORT, &motor_driver));

    // Apply duty cycle in reverse to activate power
    pb_assert(pbdrv_motor_driver_set_duty_cycle(motor_driver, -PBDRV_MOTOR_DRIVER_MAX_DUTY * duty / 100));
    #endif
}

pbio_iodev_type_id_t pb_device_get_id(pb_device_t *pbdev) {
    return pbdev->iodev.info->type_id;
}

uint8_t pb_device_get_mode(pb_device_t *pbdev) {
    return pbdev->iodev.mode;
}

uint8_t pb_device_get_num_values(pb_device_t *pbdev) {
    return pbdev->iodev.info->mode_info[pbdev->iodev.mode].num_values;
}

int8_t pb_device_get_mode_id_from_str(pb_device_t *pbdev, const char *mode_str) {
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    return 0;
}

void pb_device_setup_motor(pbio_port_id_t port, bool is_servo) {
    // HACK: Built-in motors on BOOST Move hub do not have I/O ports associated
    // with them.
    #if PYBRICKS_HUB_MOVEHUB
    if (port == PBIO_PORT_ID_A || port == PBIO_PORT_ID_B) {
        return;
    }
    #endif

    // Get the iodevice
    pbio_iodev_t *iodev;
    pbio_error_t err;

    // Set up device
    while ((err = pbdrv_ioport_get_iodev(port, &iodev)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(50);
    }
    pb_assert(err);

    // Only motors are allowed.
    if (!PBIO_IODEV_IS_DC_OUTPUT(iodev)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // If it's a DC motor, no further setup is needed.
    if (!PBIO_IODEV_IS_FEEDBACK_MOTOR(iodev)) {
        return;
    }

    // Choose mode based on device capabilities.
    uint8_t mode_id = PBIO_IODEV_IS_ABS_MOTOR(iodev) ?
        PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB:
        PBIO_IODEV_MODE_PUP_REL_MOTOR__POS;

    // Activate mode.
    set_mode(iodev, mode_id);
}

#endif // PYBRICKS_PY_PUPDEVICES
