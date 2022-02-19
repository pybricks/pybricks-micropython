// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

// Driver for motor driver chips that use an H-bridge driven by PWMs.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM

#include <pbdrv/gpio.h>
#include <pbdrv/motor_driver.h>
#include <pbdrv/pwm.h>
#include <pbio/port.h>

#include "motor_driver_hbridge_pwm.h"

pbio_error_t pbdrv_motor_driver_coast(pbio_port_id_t port) {
    const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data;

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    data = &pbdrv_motor_driver_hbridge_pwm_platform_data[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

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
        pbdrv_pwm_set_duty(pwm_dev, data->pin1_pwm_ch, duty_cycle / 10);
    }
    pbdrv_gpio_alt(&data->pin1_gpio, data->pin1_alt);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_driver_run_rev(const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *pwm_dev;
    if (pbdrv_pwm_get_dev(data->pin2_pwm_id, &pwm_dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm_dev, data->pin2_pwm_ch, duty_cycle / 10);
    }
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_alt(&data->pin2_gpio, data->pin2_alt);
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    const pbdrv_motor_driver_hbridge_pwm_platform_data_t *data;

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    if (duty_cycle < -PBDRV_MOTOR_DRIVER_MAX_DUTY || duty_cycle > PBDRV_MOTOR_DRIVER_MAX_DUTY) {
        return PBIO_ERROR_INVALID_ARG;
    }

    data = &pbdrv_motor_driver_hbridge_pwm_platform_data[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

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
    for (int i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbdrv_motor_driver_coast(i + PBDRV_CONFIG_FIRST_MOTOR_PORT);
    }
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_HBRIDGE_PWM
