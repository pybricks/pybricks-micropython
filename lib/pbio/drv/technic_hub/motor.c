// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR && !PBIO_TEST_BUILD

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/gpio.h>
#include <pbdrv/motor.h>
#include <pbdrv/pwm.h>
#include <pbio/config.h>

#include "stm32l4xx_hal.h"

typedef struct {
    pbdrv_gpio_t pin1_gpio;
    pbdrv_gpio_t pin2_gpio;
    uint32_t alt;
    uint8_t pwm_dev_id;
    uint8_t pwm_dev_ch;
} pbdrv_motor_data_t;

static const pbdrv_motor_data_t
    platform_data[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER] = {
    // Port A
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 1,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 14,
        .alt = GPIO_AF14_TIM15,
        .pwm_dev_id = 1,
        .pwm_dev_ch = 1,
    },
    // Port B
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 9,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 0,
        .alt = GPIO_AF1_TIM1,
        .pwm_dev_id = 0,
        .pwm_dev_ch = 2,
    },
    // Port C
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 13,
        .pin2_gpio.bank = GPIOA,
        .pin2_gpio.pin = 8,
        .alt = GPIO_AF1_TIM1,
        .pwm_dev_id = 0,
        .pwm_dev_ch = 1,
    },
    // Port D
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 10,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 1,
        .alt = GPIO_AF1_TIM1,
        .pwm_dev_id = 0,
        .pwm_dev_ch = 3,
    },
};

pbio_error_t pbdrv_motor_coast(pbio_port_id_t port) {
    const pbdrv_motor_data_t *data;

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    data = &platform_data[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    pbdrv_gpio_out_low(&data->pin1_gpio);
    pbdrv_gpio_out_low(&data->pin2_gpio);

    return PBIO_SUCCESS;
}

static void pbdrv_motor_brake(const pbdrv_motor_data_t *data) {
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_run_fwd(const pbdrv_motor_data_t *data, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *pwm_dev;
    if (pbdrv_pwm_get_dev(data->pwm_dev_id, &pwm_dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm_dev, data->pwm_dev_ch, duty_cycle / 10);
    }
    pbdrv_gpio_alt(&data->pin1_gpio, data->alt);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_run_rev(const pbdrv_motor_data_t *data, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *pwm_dev;
    if (pbdrv_pwm_get_dev(data->pwm_dev_id, &pwm_dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm_dev, data->pwm_dev_ch, duty_cycle / 10);
    }
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_alt(&data->pin2_gpio, data->alt);
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    const pbdrv_motor_data_t *data;

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    if (duty_cycle < -10000 || duty_cycle > 10000) {
        return PBIO_ERROR_INVALID_ARG;
    }

    data = &platform_data[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    if (duty_cycle > 0) {
        pbdrv_motor_run_fwd(data, duty_cycle);
    } else if (duty_cycle < 0) {
        pbdrv_motor_run_rev(data, -duty_cycle);
    } else {
        pbdrv_motor_brake(data);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_get_id(pbio_port_id_t port, pbio_iodev_type_id_t *id) {

    pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (!PBIO_IODEV_IS_MOTOR(iodev)) {
        return PBIO_ERROR_NO_DEV;
    }

    *id = iodev->info->type_id;

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_MOTOR
