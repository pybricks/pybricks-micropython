// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stm32f070xb.h"

#include <pbdrv/motor.h>
#include <pbio/config.h>

void _pbdrv_motor_init(void) {
    // it isn't clear what PB2 does yet, but tacho doesn't work without setting it high.
    // maybe it switches power to the IR LEDs? plus more?

    // PB2 output, high
    GPIOB->BSRR = GPIO_BSRR_BS_2;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER2_Msk) | (1 << GPIO_MODER_MODER2_Pos);

    // TIM1 provides PWM for ports A/B
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    TIM1->PSC = 3;      // divide by 4 (= 3 + 1), so ticks are 12MHz
    TIM1->ARR = 10000;  // 12MHz divided by 10k makes 1.2kHz PWM
    TIM1->BDTR |= TIM_BDTR_MOE;

    // port A
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOA->BRR = GPIO_BRR_BR_8;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
    GPIOA->BRR = GPIO_BRR_BR_10;
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (2 << GPIO_AFRH_AFSEL8_Pos);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL10_Msk) | (2 << GPIO_AFRH_AFSEL10_Pos);
    TIM1->CCR1 = 0;
    TIM1->CCR3 = 0;
    TIM1->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM mode 1
    TIM1->CCMR2 |= (6 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE; // PWM mode 1
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC3E;

    // port B
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
    GPIOA->BRR = GPIO_BRR_BR_9;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
    GPIOA->BRR = GPIO_BRR_BR_11;
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL9_Msk) | (2 << GPIO_AFRH_AFSEL9_Pos);
    GPIOA->AFR[1] = (GPIOA->AFR[1] & ~GPIO_AFRH_AFSEL11_Msk) | (2 << GPIO_AFRH_AFSEL11_Pos);
    TIM1->CCR2 = 0;
    TIM1->CCR4 = 0;
    TIM1->CCMR1 |= (6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE; // PWM mode 1
    TIM1->CCMR2 |= (6 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE; // PWM mode 1
    TIM1->CCER |= TIM_CCER_CC2E | TIM_CCER_CC4E;

    // apply settings and start timer
    TIM1->CR1 |= TIM_CR1_CEN;
    TIM1->EGR |= TIM_EGR_UG;

    // TIM3 is used for port C/D PWM
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    TIM3->PSC = 3;      // divide by 4 (= 3 + 1), so ticks are 12MHz
    TIM3->ARR = 10000;  // 12MHz divided by 10k makes 1.2kHz PWM
    TIM3->BDTR |= TIM_BDTR_MOE;

    // port C
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
    GPIOC->BRR = GPIO_BRR_BR_6;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOC->BRR = GPIO_BRR_BR_8;
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL6_Msk) | (0 << GPIO_AFRL_AFSEL6_Pos);
    GPIOC->AFR[1] = (GPIOC->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (0 << GPIO_AFRH_AFSEL8_Pos);
    TIM3->CCR1 = 0;
    TIM3->CCR3 = 0;
    TIM3->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM mode 1
    TIM3->CCER |= TIM_CCER_CC1E;
    TIM3->CCMR2 |= (6 << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE; // PWM mode 1
    TIM3->CCER |= TIM_CCER_CC3E;

    // port D
    // init PWM pins as gpio out low (coasting) and prepare alternate function
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
    GPIOC->BRR = GPIO_BRR_BR_7;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
    GPIOC->BRR = GPIO_BRR_BR_9;
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~GPIO_AFRL_AFSEL7_Msk) | (0 << GPIO_AFRL_AFSEL7_Pos);
    GPIOC->AFR[1] = (GPIOC->AFR[1] & ~GPIO_AFRH_AFSEL9_Msk) | (0 << GPIO_AFRH_AFSEL9_Pos);
    TIM3->CCR2 = 0;
    TIM3->CCR4 = 0;
    TIM3->CCMR1 |= (6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE; // PWM mode 1
    TIM3->CCER |= TIM_CCER_CC2E;
    TIM3->CCMR2 |= (6 << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE; // PWM mode 1
    TIM3->CCER |= TIM_CCER_CC4E;

    // apply settings and start timer
    TIM3->CR1 |= TIM_CR1_CEN;
    TIM3->EGR |= TIM_EGR_UG;
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
    // if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
    //     if (!get_iodev(port)) {
    //         return PBIO_ERROR_NO_DEV;
    //     }
    // }

    // set both port pins 1 and 2 to output low
    switch (port) {
    case PBIO_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOA->BRR = GPIO_BRR_BR_8;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
        GPIOA->BRR = GPIO_BRR_BR_10;
        break;
    case PBIO_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOA->BRR = GPIO_BRR_BR_9;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
        GPIOA->BRR = GPIO_BRR_BR_11;
        break;
    case PBIO_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
        GPIOC->BRR = GPIO_BRR_BR_6;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOC->BRR = GPIO_BRR_BR_8;
        break;
    case PBIO_PORT_D:
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

static void pbdrv_motor_brake(pbio_port_t port) {
    // set both port pins 1 and 2 to output high
    switch (port) {
    case PBIO_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_8;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_10;
        break;
    case PBIO_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_9;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_11;
        break;
    case PBIO_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_6;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_8;
        break;
    case PBIO_PORT_D:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_7;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_9;
        break;
    default:
        break;
    }
}

static void pbdrv_motor_run_fwd(pbio_port_t port, int16_t duty_cycle) {
    // one pin as out, high and the other as PWM
    switch (port) {
    case PBIO_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_8;
        TIM1->CCR3 = 10000 - duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (2 << GPIO_MODER_MODER10_Pos);
        break;
    case PBIO_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_11;
        TIM1->CCR2 = 10000 - duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (2 << GPIO_MODER_MODER9_Pos);
        break;
    case PBIO_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_6;
        TIM3->CCR3 = 10000 - duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
        break;
    case PBIO_PORT_D:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_9;
        TIM3->CCR2 = 10000 - duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (2 << GPIO_MODER_MODER7_Pos);
        break;
    default:
        break;
    }
}

static void pbdrv_motor_run_rev(pbio_port_t port, int16_t duty_cycle) {
    // one pin as out, high and the other as PWM
    switch (port) {
    case PBIO_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_10;
        TIM1->CCR1 = 10000 + duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
        break;
    case PBIO_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_9;
        TIM1->CCR4 = 10000 + duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (2 << GPIO_MODER_MODER11_Pos);
        break;
    case PBIO_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_8;
        TIM3->CCR1 = 10000 + duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (2 << GPIO_MODER_MODER6_Pos);
        break;
    case PBIO_PORT_D:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_7;
        TIM3->CCR4 = 10000 + duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (2 << GPIO_MODER_MODER9_Pos);
        break;
    default:
        break;
    }
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
    //     pbio_iodev_t *iodev;

    //     iodev = get_iodev(port);

    //     if (!iodev) {
    //         return PBIO_ERROR_NO_DEV;
    //     }
    // }

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

pbio_error_t pbdrv_motor_get_id(pbio_port_t port, pbio_iodev_type_id_t *id) {
    if (port == PBIO_PORT_A || port == PBIO_PORT_B) {
        *id = PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR;
        return PBIO_SUCCESS;
    }
    else  if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        pbio_iodev_t *iodev;

        iodev = get_iodev(port);

        if (!iodev) {
            return PBIO_ERROR_NO_DEV;
        }

        *id = iodev->info->type_id;

        return PBIO_SUCCESS;
    }

    return PBIO_ERROR_INVALID_PORT;
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void) {
    // disable the PWM timers
    TIM1->CR1 &= TIM_CR1_CEN;
    TIM3->CR1 &= TIM_CR1_CEN;

    // set H-bridge pins to output, low (coast)
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOA->BRR = GPIO_BRR_BR_8;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
    GPIOA->BRR = GPIO_BRR_BR_10;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
    GPIOA->BRR = GPIO_BRR_BR_9;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
    GPIOA->BRR = GPIO_BRR_BR_11;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
    GPIOC->BRR = GPIO_BRR_BR_6;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOC->BRR = GPIO_BRR_BR_8;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
    GPIOC->BRR = GPIO_BRR_BR_7;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
    GPIOC->BRR = GPIO_BRR_BR_9;

    // disable tacho irq
    NVIC_DisableIRQ(EXTI0_1_IRQn);
}
#endif

#endif // PBDRV_CONFIG_MOTOR
