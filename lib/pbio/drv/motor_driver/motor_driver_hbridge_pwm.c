// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

// Driver for motor driver chips that use an H-bridge driven by PWMs.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM

#include <assert.h>

#include <pbdrv/gpio.h>
#include <pbdrv/motor_driver.h>
#include <pbdrv/pwm.h>

#include "motor_driver_hbridge_pwm.h"

struct _pbdrv_motor_driver_dev_t {
    const pbdrv_motor_driver_hbridge_pwm_platform_data_t *pdata;
};

static pbdrv_motor_driver_dev_t motor_drivers[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV];

pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    if (id >= PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *driver = &motor_drivers[id];

    // if pdata is not set, then driver hasn't been initialized
    if (!(*driver)->pdata) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {
    const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data = driver->pdata;

    pbdrv_gpio_out_low(&data->pin1_gpio);
    pbdrv_gpio_out_low(&data->pin2_gpio);

    return PBIO_SUCCESS;
}

static void pbdrv_motor_driver_brake(const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data) {
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_driver_run_fwd(const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *pwm_dev;
    if (pbdrv_pwm_get_dev(data->pin1_pwm_id, &pwm_dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm_dev, data->pin1_pwm_ch, duty_cycle);
    }
    pbdrv_gpio_alt(&data->pin1_gpio, data->pin1_alt);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_driver_run_rev(const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *pwm_dev;
    if (pbdrv_pwm_get_dev(data->pin2_pwm_id, &pwm_dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm_dev, data->pin2_pwm_ch, duty_cycle);
    }
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_alt(&data->pin2_gpio, data->pin2_alt);
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    assert(duty_cycle >= -PBDRV_MOTOR_DRIVER_MAX_DUTY);
    assert(duty_cycle <= PBDRV_MOTOR_DRIVER_MAX_DUTY);

    const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data = driver->pdata;

    if (duty_cycle > 0) {
        pbdrv_motor_driver_run_fwd(data, duty_cycle);
    } else if (duty_cycle < 0) {
        pbdrv_motor_driver_run_rev(data, -duty_cycle);
    } else {
        pbdrv_motor_driver_brake(data);
    }

    return PBIO_SUCCESS;
}

void pbdrv_motor_driver_init(void) {
    // REVISIT: We should be getting the PWM devices during init, but this
    // requires adding a contiki process only for that, which doesn't seem
    // worth the space it takes up in firmware size (~150 bytes on move hub).
    for (int i = 0; i < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; i++) {
        motor_drivers[i].pdata = &pbdrv_motor_driver_hbridge_pwm_platform_data[i];
        pbdrv_motor_driver_coast(&motor_drivers[i]);
    }
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM
