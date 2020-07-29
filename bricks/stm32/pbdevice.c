// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <string.h>

#include <pbdrv/ioport.h>
#include <pbdrv/motor.h>

#include <pbio/color.h>
#include <pbio/iodev.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "util/pberror.h"
#include "pbdevice.h"

struct _pbdevice_t {
    pbio_iodev_t iodev;
};

static void wait(pbio_error_t (*end)(pbio_iodev_t *), void (*cancel)(pbio_iodev_t *), pbio_iodev_t *iodev) {
    nlr_buf_t nlr;
    pbio_error_t err;

    if (nlr_push(&nlr) == 0) {
        while ((err = end(iodev)) == PBIO_ERROR_AGAIN) {
            MICROPY_EVENT_POLL_HOOK
        }
        nlr_pop();
        pb_assert(err);
    } else {
        cancel(iodev);
        while (end(iodev) == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP
        }
        nlr_jump(nlr.ret_val);
    }
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
        ;
    }
    pb_assert(err);
    wait(pbio_iodev_set_mode_end, pbio_iodev_set_mode_cancel, iodev);

    // Give some time for the mode to take effect and discard stale data
    uint32_t delay = get_mode_switch_delay(iodev->info->type_id, new_mode);
    if (delay > 0) {
        mp_hal_delay_ms(delay);
    }
}

pbdevice_t *pbdevice_get_device(pbio_port_t port, pbio_iodev_type_id_t valid_id) {

    // Get the iodevice
    pbio_iodev_t *iodev;
    pb_assert(pbdrv_ioport_get_iodev(port, &iodev));

    // Is there an IO Device?
    if (!iodev->info) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Verify the ID or always allow generic LUMP device
    if (iodev->info->type_id != valid_id && valid_id != PBIO_IODEV_TYPE_ID_LUMP_UART) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Return pointer to device
    iodev->port = port;
    return (pbdevice_t *)iodev;
}

void pbdevice_get_values(pbdevice_t *pbdev, uint8_t mode, int32_t *values) {

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
        switch (type) {
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

void pbdevice_set_values(pbdevice_t *pbdev, uint8_t mode, int32_t *values, uint8_t num_values) {

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
        switch (type) {
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
        ;
    }
    pb_assert(err);
    wait(pbio_iodev_set_data_end, pbio_iodev_set_data_cancel, iodev);
}

void pbdevice_set_power_supply(pbdevice_t *pbdev, int32_t duty) {
    // Bind user input to percentage
    if (duty < 0) {
        duty = 0;
    } else if (duty > 100) {
        duty = 100;
    }
    // Apply duty cycle in reverse to activate power
    pb_assert(pbdrv_motor_set_duty_cycle(pbdev->iodev.port, -100 * duty));
}

void pbdevice_get_info(pbdevice_t *pbdev, pbio_port_t *port, pbio_iodev_type_id_t *id, uint8_t *mode, uint8_t *num_values) {
    *port = pbdev->iodev.port;
    *id = pbdev->iodev.info->type_id;
    *mode = pbdev->iodev.mode;
    *num_values = pbdev->iodev.info->mode_info[*mode].num_values;
}

int8_t pbdevice_get_mode_id_from_str(pbdevice_t *pbdev, const char *mode_str) {
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    return 0;
}

void pbdevice_color_light_on(pbdevice_t *pbdev, pbio_color_t color) {
    // Turn on the light through device specific mode
    uint8_t mode;
    switch (pbdev->iodev.info->type_id) {
        case PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR:
            switch (color) {
                case PBIO_COLOR_GREEN:
                    mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX;
                    break;
                case PBIO_COLOR_RED:
                    mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__REFLT;
                    break;
                case PBIO_COLOR_BLUE:
                    mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI;
                    break;
                default:
                    mode = PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1;
                    break;
            }
            int32_t data[4];
            pbdevice_get_values(pbdev, mode, data);
            break;
        default:
            pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    }
}
