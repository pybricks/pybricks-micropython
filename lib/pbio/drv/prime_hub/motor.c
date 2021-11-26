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

#include "stm32f4xx_hal.h"

typedef struct {
    pbdrv_gpio_t pin1_gpio;
    uint32_t pin1_alt;
    uint8_t pin1_pwm_id;
    uint8_t pin1_pwm_ch;
    pbdrv_gpio_t pin2_gpio;
    uint32_t pin2_alt;
    uint8_t pin2_pwm_id;
    uint8_t pin2_pwm_ch;
} pbdrv_motor_data_t;

static const pbdrv_motor_data_t
    platform_data[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER] = {
    // Port A
    {
        .pin1_gpio.bank = GPIOE,
        .pin1_gpio.pin = 9,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = 0,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOE,
        .pin2_gpio.pin = 11,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = 0,
        .pin2_pwm_ch = 2,
    },
    // Port B
    {
        .pin1_gpio.bank = GPIOE,
        .pin1_gpio.pin = 13,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = 0,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOE,
        .pin2_gpio.pin = 14,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = 0,
        .pin2_pwm_ch = 4,
    },
    // Port C
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 6,
        .pin1_alt = GPIO_AF2_TIM4,
        .pin1_pwm_id = 2,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 7,
        .pin2_alt = GPIO_AF2_TIM4,
        .pin2_pwm_id = 2,
        .pin2_pwm_ch = 2,
    },
    // Port D
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 8,
        .pin1_alt = GPIO_AF2_TIM4,
        .pin1_pwm_id = 2,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 9,
        .pin2_alt = GPIO_AF2_TIM4,
        .pin2_pwm_id = 2,
        .pin2_pwm_ch = 4,
    },
    // Port E
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 6,
        .pin1_alt = GPIO_AF2_TIM3,
        .pin1_pwm_id = 1,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOC,
        .pin2_gpio.pin = 7,
        .pin2_alt = GPIO_AF2_TIM3,
        .pin2_pwm_id = 1,
        .pin2_pwm_ch = 2,
    },
    // Port F
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 8,
        .pin1_alt = GPIO_AF2_TIM3,
        .pin1_pwm_id = 1,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 1,
        .pin2_alt = GPIO_AF2_TIM3,
        .pin2_pwm_id = 1,
        .pin2_pwm_ch = 4,
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
    if (pbdrv_pwm_get_dev(data->pin1_pwm_id, &pwm_dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm_dev, data->pin1_pwm_ch, duty_cycle / 10);
    }
    pbdrv_gpio_alt(&data->pin1_gpio, data->pin1_alt);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_run_rev(const pbdrv_motor_data_t *data, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *pwm_dev;
    if (pbdrv_pwm_get_dev(data->pin2_pwm_id, &pwm_dev) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm_dev, data->pin2_pwm_ch, duty_cycle / 10);
    }
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_alt(&data->pin2_gpio, data->pin2_alt);
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
    if (err == PBIO_ERROR_NO_DEV) {
        *id = PBIO_IODEV_TYPE_ID_NONE;
        return PBIO_SUCCESS;
    } else if (err != PBIO_SUCCESS) {
        return err;
    }

    if (!PBIO_IODEV_IS_DC_OUTPUT(iodev)) {
        return PBIO_ERROR_NO_DEV;
    }

    *id = iodev->info->type_id;

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_MOTOR
