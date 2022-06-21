// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER

#include <stdint.h>

#include <pbdrv/counter.h>
#include <pbio/error.h>

#include "counter_ev3dev_stretch_iio.h"
#include "counter_lpf2.h"
#include "counter_nxt.h"
#include "counter_stm32f0_gpio_quad_enc.h"
#include "counter_test.h"
#include "counter_virtual.h"
#include "counter.h"

static pbdrv_counter_dev_t pbdrv_counter_devs[PBDRV_CONFIG_COUNTER_NUM_DEV];

void pbdrv_counter_init(void) {
    pbdrv_counter_ev3dev_stretch_iio_init(pbdrv_counter_devs);
    pbdrv_counter_lpf2_init(pbdrv_counter_devs);
    pbdrv_counter_nxt_init(pbdrv_counter_devs);
    pbdrv_counter_stm32f0_gpio_quad_enc_init(pbdrv_counter_devs);
    pbdrv_counter_test_init(pbdrv_counter_devs);
    pbdrv_counter_virtual_init(pbdrv_counter_devs);
}

pbio_error_t pbdrv_counter_get_dev(uint8_t id, pbdrv_counter_dev_t **dev) {
    if (id >= PBDRV_CONFIG_COUNTER_NUM_DEV) {
        return PBIO_ERROR_NO_DEV;
    }

    *dev = &pbdrv_counter_devs[id];

    if ((*dev)->funcs == NULL) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Gets the current angle as a pair of whole rotations and millidegrees.
 *
 * The implementation may choose how the angle is distributed across the output
 * values, as long as the total angle equals:
 *
 *     rotations * 360 000 + millidegrees.
 *
 * @param [in]  dev           Pointer to the counter device
 * @param [out] rotations     Returns whole rotaions on success
 * @param [out] millidegrees  Returns the count on success
 * @return                    ::PBIO_SUCCESS on success, ::PBIO_ERROR_NO_DEV if
 *                            the counter has not been initialized,
 *                            ::PBIO_ERROR_NOT_SUPPORTED if the counter driver
 *                            is disabled.
 */
pbio_error_t pbdrv_counter_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {
    return dev->funcs->get_angle(dev, rotations, millidegrees);
}

/**
 * Gets the absolute angle if the counter supports it.
 * @param [in]  dev            Pointer to the counter device
 * @param [out] millidegrees   Returns the angle on success
 * @return                     ::PBIO_SUCCESS on success, ::PBIO_ERROR_NO_DEV
 *                             if the counter has not been initialized,
 *                             ::PBIO_ERROR_NOT_SUPPORTED if this counter does
 *                             not support reading the absolute count or the
 *                             counter driver is disabled.
 */
pbio_error_t pbdrv_counter_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {
    if (!dev->funcs->get_abs_angle) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    return dev->funcs->get_abs_angle(dev, millidegrees);
}

#endif // PBDRV_CONFIG_COUNTER
