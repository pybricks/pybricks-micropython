// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/pwm.h>
#include <pbio/error.h>

#include "pwm_stm32_tim.h"
#include "pwm_test.h"
#include "pwm_tlc5955_stm32.h"
#include "pwm.h"


static pbdrv_pwm_dev_t pbdrv_pwm_dev[PBDRV_CONFIG_PWM_NUM_DEV];

/**
 * Initializes all PWM drivers.
 */
void pbdrv_pwm_init(void) {
    pbdrv_pwm_stm32_tim_init(pbdrv_pwm_dev);
    pbdrv_pwm_test_init(pbdrv_pwm_dev);
    pbdrv_pwm_tlc5955_stm32_init(pbdrv_pwm_dev);
}

/**
 * Gets the PWM device with the given ID.
 * @param [in]  id      The ID of the PWM device.
 * @param [out] dev     Pointer to hold the returned PWM device.
 * @return              ::PBIO_SUCCESS on success, ::PBIO_ERROR_NO_DEV if
 *                      the ID is not valid, ::PBIO_ERROR_AGAIN if the PWM
 *                      device has not been intialized yet or
 *                      ::PBIO_ERROR_NOT_SUPPORTED if the PWM driver is
 *                      disabled.
 */
pbio_error_t pbdrv_pwm_get_dev(uint8_t id, pbdrv_pwm_dev_t **dev) {
    if (id >= PBDRV_CONFIG_PWM_NUM_DEV) {
        return PBIO_ERROR_NO_DEV;
    }

    *dev = &pbdrv_pwm_dev[id];

    if ((*dev)->funcs == NULL) {
        // has not been intialized yet
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Sets the duty cycle for a PWM channel.
 * @param [in]  dev     Pointer to the PWM device.
 * @param [in]  ch      The channel descriptor.
 * @param [in ] value   The duty cycle value in counts (driver-specific)
 * @return              ::PBIO_SUCCESS on success or ::PBIO_ERROR_NOT_SUPPORTED
 *                      if the PWM driver is disabled.
 */
pbio_error_t pbdrv_pwm_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    return dev->funcs->set_duty(dev, ch, value);
}

#endif // PBDRV_CONFIG_PWM
