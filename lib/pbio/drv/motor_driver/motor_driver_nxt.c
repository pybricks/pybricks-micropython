// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020,2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_NXT

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/error.h>

#include "../rproc/rproc_nxt.h"

struct _pbdrv_motor_driver_dev_t {
    uint8_t index;
};

static pbdrv_motor_driver_dev_t motor_ports[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    [0 ... PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV - 1] = { .index = UINT8_MAX },
};

pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    if (id >= PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *driver = &motor_ports[id];

    // if port has not been set, then driver has not been initialized
    if ((*driver)->index == UINT8_MAX) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {
    return pbdrv_rproc_nxt_set_duty_cycle(driver->index, 0, false);
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    return pbdrv_rproc_nxt_set_duty_cycle(driver->index, 100 * duty_cycle / PBDRV_MOTOR_DRIVER_MAX_DUTY, true);
}

void pbdrv_motor_driver_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; i++) {
        motor_ports[i].index = i;
    }
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_NXT
