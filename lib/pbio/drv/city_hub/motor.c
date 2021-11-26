// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR && !PBIO_TEST_BUILD

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stm32f030xc.h"

#include <pbdrv/motor.h>
#include <pbdrv/pwm.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

pbio_error_t pbdrv_motor_coast(pbio_port_id_t port) {

    // set both port pins 1 and 2 to output low
    switch (port) {
        case PBIO_PORT_ID_B:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
            GPIOC->BRR = GPIO_BRR_BR_6;
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
            GPIOC->BRR = GPIO_BRR_BR_8;
            break;
        case PBIO_PORT_ID_A:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
            GPIOC->BRR = GPIO_BRR_BR_7;
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
            GPIOC->BRR = GPIO_BRR_BR_9;
            break;
        default:
            return PBIO_ERROR_INVALID_PORT;
    }

    return PBIO_SUCCESS;
}

static void pbdrv_motor_brake(pbio_port_id_t port) {
    // set both port pins 1 and 2 to output high
    switch (port) {
        case PBIO_PORT_ID_B:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_6;
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_8;
            break;
        case PBIO_PORT_ID_A:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_7;
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_9;
            break;
        default:
            break;
    }
}

static void pbdrv_motor_run_fwd(pbio_port_id_t port, int16_t duty_cycle) { // duty is pos
    pbdrv_pwm_dev_t *dev;

    // one pin as out, high and the other as PWM
    switch (port) {
        case PBIO_PORT_ID_B:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_6;
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 3, 1000 - duty_cycle / 10);
            }
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
            break;
        case PBIO_PORT_ID_A:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_7;
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 4, 1000 - duty_cycle / 10);
            }
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (2 << GPIO_MODER_MODER9_Pos);
            break;
        default:
            break;
    }
}

static void pbdrv_motor_run_rev(pbio_port_id_t port, int16_t duty_cycle) { // duty is neg
    pbdrv_pwm_dev_t *dev;

    // one pin as out, high and the other as PWM
    switch (port) {
        case PBIO_PORT_ID_B:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_8;
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 1, 1000 + duty_cycle / 10);
            }
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (2 << GPIO_MODER_MODER6_Pos);
            break;
        case PBIO_PORT_ID_A:
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
            GPIOC->BSRR = GPIO_BSRR_BS_9;
            if (pbdrv_pwm_get_dev(0, &dev) == PBIO_SUCCESS) {
                pbdrv_pwm_set_duty(dev, 2, 1000 + duty_cycle / 10);
            }
            GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (2 << GPIO_MODER_MODER7_Pos);
            break;
        default:
            break;
    }
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    if (port < PBIO_PORT_ID_A || port > PBIO_PORT_ID_B) {
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
