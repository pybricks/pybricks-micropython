// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL

#include <stdint.h>

#include <pbdrv/clock.h>
#include <pbdrv/motor_driver.h>

#include "../virtual.h"


struct _pbdrv_motor_driver_dev_t {
    uint8_t id;
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
    return pbdrv_virtual_call_method("motor_driver", driver->id, "on_coast", "(I)", pbdrv_clock_get_us());
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    return pbdrv_virtual_call_method("motor_driver", driver->id, "on_set_duty_cycle", "Id",
        pbdrv_clock_get_us(), (double)duty_cycle / (double)PBDRV_MOTOR_DRIVER_MAX_DUTY);
}

void pbdrv_motor_driver_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; i++) {
        motor_driver_devs[i].id = i;
    }
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_VIRTUAL
