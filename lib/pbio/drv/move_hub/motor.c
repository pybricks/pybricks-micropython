// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR && !PBIO_TEST_BUILD

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stm32f070xb.h"

#include <pbdrv/gpio.h>
#include <pbdrv/motor.h>
#include <pbdrv/pwm.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

static const pbdrv_gpio_t port_a_pin_1 = { .bank = GPIOA, .pin = 8 };
static const pbdrv_gpio_t port_a_pin_2 = { .bank = GPIOA, .pin = 10 };
static const pbdrv_gpio_t port_b_pin_1 = { .bank = GPIOA, .pin = 9 };
static const pbdrv_gpio_t port_b_pin_2 = { .bank = GPIOA, .pin = 11 };
static const pbdrv_gpio_t port_c_pin_1 = { .bank = GPIOC, .pin = 8 };
static const pbdrv_gpio_t port_c_pin_2 = { .bank = GPIOC, .pin = 6 };
static const pbdrv_gpio_t port_d_pin_1 = { .bank = GPIOC, .pin = 7 };
static const pbdrv_gpio_t port_d_pin_2 = { .bank = GPIOC, .pin = 9 };

pbio_error_t pbdrv_motor_coast(pbio_port_id_t port) {
    // motor terminals will be floating when both pins are low

    switch (port) {
        case PBIO_PORT_ID_A:
            pbdrv_gpio_out_low(&port_a_pin_1);
            pbdrv_gpio_out_low(&port_a_pin_2);
            break;
        case PBIO_PORT_ID_B:
            pbdrv_gpio_out_low(&port_b_pin_1);
            pbdrv_gpio_out_low(&port_b_pin_2);
            break;
        case PBIO_PORT_ID_C:
            pbdrv_gpio_out_low(&port_c_pin_1);
            pbdrv_gpio_out_low(&port_c_pin_2);
            break;
        case PBIO_PORT_ID_D:
            pbdrv_gpio_out_low(&port_d_pin_1);
            pbdrv_gpio_out_low(&port_d_pin_2);
            break;
        default:
            return PBIO_ERROR_INVALID_PORT;
    }

    return PBIO_SUCCESS;
}

static void pbdrv_motor_brake(pbio_port_id_t port) {
    // motor terminals will short-circuit when both pins are high

    switch (port) {
        case PBIO_PORT_ID_A:
            pbdrv_gpio_out_high(&port_a_pin_1);
            pbdrv_gpio_out_high(&port_a_pin_2);
            break;
        case PBIO_PORT_ID_B:
            pbdrv_gpio_out_high(&port_b_pin_1);
            pbdrv_gpio_out_high(&port_b_pin_2);
            break;
        case PBIO_PORT_ID_C:
            pbdrv_gpio_out_high(&port_c_pin_1);
            pbdrv_gpio_out_high(&port_c_pin_2);
            break;
        case PBIO_PORT_ID_D:
            pbdrv_gpio_out_high(&port_d_pin_1);
            pbdrv_gpio_out_high(&port_d_pin_2);
            break;
        default:
            break;
    }
}

static void pbdrv_motor_run_fwd(pbio_port_id_t port, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *dev;

    // one pin as out, high and the other as PWM
    switch (port) {
        case PBIO_PORT_ID_A:
            pbdrv_gpio_out_high(&port_a_pin_2);
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 1, 1000 - duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_a_pin_1, 2);
            break;
        case PBIO_PORT_ID_B:
            pbdrv_gpio_out_high(&port_b_pin_2);
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 2, 1000 - duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_b_pin_1, 2);
            break;
        case PBIO_PORT_ID_C:
            pbdrv_gpio_out_high(&port_c_pin_2);
            if (pbdrv_pwm_get_dev(1, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 3, 1000 - duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_c_pin_1, 0);
            break;
        case PBIO_PORT_ID_D:
            pbdrv_gpio_out_high(&port_d_pin_2);
            if (pbdrv_pwm_get_dev(1, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 2, 1000 - duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_d_pin_1, 0);
            break;
        default:
            break;
    }
}

static void pbdrv_motor_run_rev(pbio_port_id_t port, int16_t duty_cycle) {
    pbdrv_pwm_dev_t *dev;

    // one pin as out, high and the other as PWM
    switch (port) {
        case PBIO_PORT_ID_A:
            pbdrv_gpio_out_high(&port_a_pin_1);
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 3, 1000 + duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_a_pin_2, 2);
            break;
        case PBIO_PORT_ID_B:
            pbdrv_gpio_out_high(&port_b_pin_1);
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 4, 1000 + duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_b_pin_2, 2);
            break;
        case PBIO_PORT_ID_C:
            pbdrv_gpio_out_high(&port_c_pin_1);
            if (pbdrv_pwm_get_dev(1, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 1, 1000 + duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_c_pin_2, 0);
            break;
        case PBIO_PORT_ID_D:
            pbdrv_gpio_out_high(&port_d_pin_1);
            if (pbdrv_pwm_get_dev(1, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 4, 1000 + duty_cycle / 10);
            }
            pbdrv_gpio_alt(&port_d_pin_2, 0);
            break;
        default:
            break;
    }
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    if (port < PBIO_PORT_ID_A || port > PBIO_PORT_ID_D) {
        return PBIO_ERROR_INVALID_PORT;
    }

    if (duty_cycle < -10000 || duty_cycle > 10000) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (duty_cycle > 0) {
        pbdrv_motor_run_fwd(port, duty_cycle);
    } else if (duty_cycle < 0) {
        pbdrv_motor_run_rev(port, duty_cycle);
    } else {
        pbdrv_motor_brake(port);
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_get_id(pbio_port_id_t port, pbio_iodev_type_id_t *id) {
    if (port == PBIO_PORT_ID_A || port == PBIO_PORT_ID_B) {
        *id = PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR;
        return PBIO_SUCCESS;
    } else if (port == PBIO_PORT_ID_C || port == PBIO_PORT_ID_D) {
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

    return PBIO_ERROR_INVALID_PORT;
}

#endif // PBDRV_CONFIG_MOTOR
