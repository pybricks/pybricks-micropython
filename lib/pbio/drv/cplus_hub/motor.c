// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/gpio.h>
#include <pbdrv/motor.h>
#include <pbio/config.h>

#include "stm32l4xx_hal.h"

// timers have to be share with status light PWM
extern TIM_HandleTypeDef *cplus_hub_htim1;
extern TIM_HandleTypeDef *cplus_hub_htim15;

typedef struct {
    pbdrv_gpio_t pin1_gpio;
    pbdrv_gpio_t pin2_gpio;
    uint32_t alt;
    volatile uint32_t *tim_ccr;
} pbdrv_motor_data_t;

static pbdrv_motor_data_t
platform_data[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER] = {
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 1,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 14,
        .alt = 14,
        .tim_ccr = &TIM15->CCR1,
    },
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 9,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 0,
        .alt = 1,
        .tim_ccr = &TIM1->CCR2,
    },
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 13,
        .pin2_gpio.bank = GPIOA,
        .pin2_gpio.pin = 8,
        .alt = 1,
        .tim_ccr = &TIM1->CCR1,
    },
    {
        .pin1_gpio.bank = GPIOA,
        .pin1_gpio.pin = 10,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 1,
        .alt = 1,
        .tim_ccr = &TIM1->CCR3,
    },
};

void _pbdrv_motor_init(void) {
    TIM_OC_InitTypeDef tim_oc_init = { 0 };

    tim_oc_init.OCMode = TIM_OCMODE_PWM1;
    tim_oc_init.OCPolarity = TIM_OCPOLARITY_LOW;

    pbdrv_motor_coast(PBIO_PORT_A);
    HAL_TIM_PWM_ConfigChannel(cplus_hub_htim15, &tim_oc_init, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(cplus_hub_htim15, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(cplus_hub_htim15, TIM_CHANNEL_1);

    pbdrv_motor_coast(PBIO_PORT_B);
    HAL_TIM_PWM_ConfigChannel(cplus_hub_htim1, &tim_oc_init, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(cplus_hub_htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(cplus_hub_htim1, TIM_CHANNEL_2);

    pbdrv_motor_coast(PBIO_PORT_C);
    HAL_TIM_PWM_ConfigChannel(cplus_hub_htim1, &tim_oc_init, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(cplus_hub_htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(cplus_hub_htim1, TIM_CHANNEL_1);

    pbdrv_motor_coast(PBIO_PORT_D);
    HAL_TIM_PWM_ConfigChannel(cplus_hub_htim1, &tim_oc_init, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(cplus_hub_htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(cplus_hub_htim1, TIM_CHANNEL_3);
}

static pbio_iodev_t *get_iodev(pbio_port_t port) {
    pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return NULL;
    }

    if (!PBIO_IODEV_IS_MOTOR(iodev)) {
        return NULL;
    }

    return iodev;
}

pbio_error_t pbdrv_motor_coast(pbio_port_t port) {
    pbdrv_motor_data_t *data;

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    data = &platform_data[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    pbdrv_gpio_out_low(&data->pin1_gpio);
    pbdrv_gpio_out_low(&data->pin2_gpio);

    return PBIO_SUCCESS;
}

static void pbdrv_motor_brake(pbdrv_motor_data_t *data) {
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_run_fwd(pbdrv_motor_data_t *data, int16_t duty_cycle) {
    *data->tim_ccr = duty_cycle;
    pbdrv_gpio_alt(&data->pin1_gpio, data->alt);
    pbdrv_gpio_out_high(&data->pin2_gpio);
}

static void pbdrv_motor_run_rev(pbdrv_motor_data_t *data, int16_t duty_cycle) {
    *data->tim_ccr = duty_cycle;
    pbdrv_gpio_out_high(&data->pin1_gpio);
    pbdrv_gpio_alt(&data->pin2_gpio, data->alt);
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    pbdrv_motor_data_t *data;

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

pbio_error_t pbdrv_motor_get_id(pbio_port_t port, pbio_iodev_type_id_t *id) {
    pbio_iodev_t *iodev;

    iodev = get_iodev(port);

    if (!iodev) {
        return PBIO_ERROR_NO_DEV;
    }

    *id = iodev->info->type_id;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_setup(pbio_port_t port, bool is_servo) {
    return PBIO_SUCCESS;
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void) {
    HAL_TIMEx_PWMN_Stop(cplus_hub_htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(cplus_hub_htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Stop(cplus_hub_htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(cplus_hub_htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Stop(cplus_hub_htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(cplus_hub_htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Stop(cplus_hub_htim15, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(cplus_hub_htim15, TIM_CHANNEL_1);
}
#endif

#endif // PBDRV_CONFIG_MOTOR
