// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stm32f030xc.h"

#include <pbdrv/motor.h>

#define PBIO_MOTOR_BUF_SIZE 32 // must be power of 2!

typedef struct {
    int32_t counts[PBIO_MOTOR_BUF_SIZE];
    uint16_t timestamps[PBIO_MOTOR_BUF_SIZE];
    int32_t count;
    uint8_t head;
} pbdrv_motor_tacho_data_t;

// only for ports A/B
static pbdrv_motor_tacho_data_t pbdrv_motor_tacho_data[2];

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

    // init port A/B tacho pins as inputs

    // port A
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER1_Msk) | (0 << GPIO_MODER_MODER1_Pos); // interupt
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER9_Msk) | (0 << GPIO_MODER_MODER9_Pos); // direction
    GPIOB->PUPDR = (GPIOB->PUPDR & ~GPIO_PUPDR_PUPDR9_Msk) | (2 << GPIO_PUPDR_PUPDR9_Pos); // pull down

    // port B
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER0_Msk) | (0 << GPIO_MODER_MODER0_Pos); // interrupt
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER1_Msk) | (0 << GPIO_MODER_MODER1_Pos); // direction
    GPIOA->PUPDR = (GPIOA->PUPDR & ~GPIO_PUPDR_PUPDR1_Msk) | (2 << GPIO_PUPDR_PUPDR1_Pos); // pull down

    // assign tacho pins to interrupts
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI0_PA | SYSCFG_EXTICR1_EXTI1_PB;
    EXTI->IMR |= EXTI_EMR_MR0 | EXTI_EMR_MR1;
    EXTI->RTSR |= EXTI_RTSR_RT0 | EXTI_RTSR_RT1;
    EXTI->FTSR |= EXTI_FTSR_FT0 | EXTI_FTSR_FT1;
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_SetPriority(EXTI0_1_IRQn, 0);

    // TIM7 is used for clock in speed measurement
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 479;    // divide 48MHz by 480 (= 479 + 1) to get 100kHz clock.
    TIM7->CR1 = TIM_CR1_CEN;
    TIM7->DIER = TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM7_IRQn);
    NVIC_SetPriority(TIM7_IRQn, 128);

    // TIM3 is used for port C PWM
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

static void pbdrv_motor_tacho_update_count(pbio_port_t port, bool int_pin_state, bool dir_pin_state, uint16_t timestamp) {
    pbdrv_motor_tacho_data_t *data;

    data = &pbdrv_motor_tacho_data[port - PBIO_PORT_A];

    if (int_pin_state ^ dir_pin_state) {
        data->count--;
    }
    else {
        data->count++;
    }

    // log timestamp on rising edge for rate calculation
    if (int_pin_state) {
        uint8_t new_head = (data->head + 1) & (PBIO_MOTOR_BUF_SIZE - 1);

        data->counts[new_head] = data->count;
        data->timestamps[new_head] = timestamp;
        data->head = new_head;
    }
}

// irq handler name defined in startup_stm32f0.s
void EXTI0_1_IRQHandler(void) {
    uint32_t exti_pr;
    uint32_t gpio_idr;
    uint16_t timestamp;
    bool int_pin_state;
    bool dir_pin_state;

    exti_pr = EXTI->PR & (EXTI_PR_PR0 | EXTI_PR_PR1);
    EXTI->PR = exti_pr; // clear the events we are handling

    timestamp = TIM7->CNT;

    // port A
    if (exti_pr & EXTI_PR_PR1) {
        gpio_idr = GPIOB->IDR;
        int_pin_state = !!(gpio_idr & GPIO_IDR_1);
        dir_pin_state = !!(gpio_idr & GPIO_IDR_9);
        pbdrv_motor_tacho_update_count(PBIO_PORT_A, int_pin_state, dir_pin_state, timestamp);
    }

    // port B
    if (exti_pr & EXTI_PR_PR0) {
        gpio_idr = GPIOA->IDR;
        int_pin_state = !!(gpio_idr & GPIO_IDR_0);
        dir_pin_state = !!(gpio_idr & GPIO_IDR_1);
        pbdrv_motor_tacho_update_count(PBIO_PORT_B, int_pin_state, dir_pin_state, timestamp);
    }
}

void TIM7_IRQHandler(void) {
    pbdrv_motor_tacho_data_t *data;
    uint16_t timestamp;
    uint8_t i, new_head;

    TIM7->SR &= ~TIM_SR_UIF; // clear interrupt

    timestamp = TIM7->CNT;

    // log a new timestamp when the timer recycles to avoid rate calculation
    // problems when the motor is not moving
    for (i = 0; i < 2; i++) {
        data = &pbdrv_motor_tacho_data[i];
        new_head = (data->head + 1) & (PBIO_MOTOR_BUF_SIZE - 1);
        data->counts[new_head] = data->count;
        data->timestamps[new_head] = timestamp;
        data->head = new_head;
    }
}

static pbio_iodev_t *get_iodev(pbio_port_t port) {
    pbio_iodev_t *iodev;
    pbio_error_t err;

    err = pbdrv_ioport_get_iodev(port, &iodev);
    if (err != PBIO_SUCCESS) {
        return NULL;
    }

    if (!(iodev->flags & PBIO_IODEV_FLAG_IS_MOTOR)) {
        return NULL;
    }

    return iodev;
}

pbio_error_t pbdrv_motor_get_encoder_count(pbio_port_t port, int32_t *count) {
    int index = port - PBIO_PORT_A;

    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }

    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        pbio_iodev_t *iodev;

        iodev = get_iodev(port);

        if (!iodev) {
            return PBIO_ERROR_NO_DEV;
        }

        // *sigh*, unaligned 32-bit value
        memcpy(count, iodev->bin_data + 1, 4);

        return PBIO_SUCCESS;
    }

    *count = pbdrv_motor_tacho_data[index].count;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    pbdrv_motor_tacho_data_t *data;
    int32_t head_count, tail_count = 0;
    uint16_t now, head_time, tail_time = 0;
    uint8_t head, tail, x = 0;

    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }

    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        pbio_iodev_t *iodev;

        iodev = get_iodev(port);

        if (!iodev) {
            return PBIO_ERROR_NO_DEV;
        }

        // scaling factor of 14 determined empirically
        *rate = *(int8_t *)iodev->bin_data * 14;

        return PBIO_SUCCESS;
    }

    // TODO: get port C/D motor speed from UART data if motor is attached
    // or return PBIO_ERROR_NO_DEV if motor is not attached

    data = &pbdrv_motor_tacho_data[port - PBIO_PORT_A];
    // head can be updated in interrupt, so only read it once
    head = data->head;
    head_count = data->counts[head];
    head_time = data->timestamps[head];

    now = TIM7->CNT;

    // if it has been more than 50ms since last timestamp, we are not moving.
    if ((uint16_t)(now - head_time) > 50 * 100) {
        *rate = 0;
        return PBIO_SUCCESS;
    }

     while (x++ < PBIO_MOTOR_BUF_SIZE) {
        tail = (head - x) & (PBIO_MOTOR_BUF_SIZE - 1);

        tail_count = data->counts[tail];
        tail_time = data->timestamps[tail];

        // if count hasn't changed, then we are not moving
        if (head_count == tail_count) {
            *rate = 0;
            return PBIO_SUCCESS;
        }

        /*
         * we need delta_t to be >= 20ms to be reasonably accurate.
         * timer is 10us, thus * 100 to get ms.
         */
        if ((uint16_t)(head_time - tail_time) >= 20 * 100) {
            break;
        }
    }

    /* avoid divide by 0 - motor probably hasn't moved yet */
    if (head_time == tail_time) {
        *rate = 0;
        return PBIO_SUCCESS;
    }

    /* timer is 100000kHz */
    *rate = (head_count - tail_count) * 100000 / (uint16_t)(head_time - tail_time);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_coast(pbio_port_t port) {
    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        if (!get_iodev(port)) {
            return PBIO_ERROR_NO_DEV;
        }
    }

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

    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        pbio_iodev_t *iodev;

        iodev = get_iodev(port);

        if (!iodev) {
            return PBIO_ERROR_NO_DEV;
        }
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

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void) {
    // disable the PWM timers
    TIM1->CR1 &= TIM_CR1_CEN;
    TIM3->CR1 &= TIM_CR1_CEN;
    TIM7->CR1 &= TIM_CR1_CEN;

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
