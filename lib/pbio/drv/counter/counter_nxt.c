// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2025 The Pybricks Authors

// GPIO Quadrature Encoder Counter driver
//
// This driver uses GPIOs and a timer to create a quadrature encoder.
//
// This driver is currently hard-coded for the LEGO BOOST Move hub internal motors.
// Ideally, this could be made into a generic driver if the following are done:
// - add IRQ support to the gpio driver
// - create a generic timer driver

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_NXT

#include <stdbool.h>
#include <stdint.h>

#include <lego/device.h>

#include <pbdrv/counter.h>
#include <pbdrv/gpio.h>
#include <pbio/util.h>

#include <nxos/drivers/motors.h>

struct _pbdrv_counter_dev_t {
    uint8_t index;
};

static pbdrv_counter_dev_t counters[PBDRV_CONFIG_COUNTER_NUM_DEV];

pbio_error_t pbdrv_counter_get_dev(uint8_t id, pbdrv_counter_dev_t **dev) {
    if (id >= PBIO_ARRAY_SIZE(counters)) {
        return PBIO_ERROR_NO_DEV;
    }
    *dev = &counters[id];
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_counter_assert_type(pbdrv_counter_dev_t *dev, lego_device_type_id_t *expected_type_id) {
    // NXT cannot detect device ID, so pass any and set NXT as the most likely.
    switch (*expected_type_id) {
        case LEGO_DEVICE_TYPE_ID_ANY_ENCODED_MOTOR:
        case LEGO_DEVICE_TYPE_ID_EV3_LARGE_MOTOR:
        case LEGO_DEVICE_TYPE_ID_EV3_MEDIUM_MOTOR:
        case LEGO_DEVICE_TYPE_ID_NXT_MOTOR:
            *expected_type_id = LEGO_DEVICE_TYPE_ID_NXT_MOTOR;
            return PBIO_SUCCESS;
        default:
            return PBIO_ERROR_NO_DEV;
    }
}

pbio_error_t pbdrv_counter_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {
    uint32_t degrees = nx_motors_get_tach_count(dev->index);
    *millidegrees = (degrees % 360) * 1000;
    *rotations = degrees / 360;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_counter_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

void pbdrv_counter_init(void) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(counters); i++) {
        pbdrv_counter_dev_t *dev = &counters[i];
        dev->index = i;
    }
}

#endif // PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
