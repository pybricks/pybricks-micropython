// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2022 The Pybricks Authors


#include <stdint.h>
#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbdrv/motor_driver.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/math.h>
#include <test-pbio.h>

#include "../src/processes.h"

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

    driver->duty_cycle = pbio_math_abs(duty_cycle);

    return PBIO_SUCCESS;
}

void pbdrv_motor_driver_init(void) {
}

// I/O port driver implementation

pbio_error_t pbdrv_ioport_get_motor_device_type_id(pbio_port_id_t id, pbio_iodev_type_id_t *type_id) {
    // default value if environment variable is not set
    *type_id = PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR;
    return PBIO_SUCCESS;
}
