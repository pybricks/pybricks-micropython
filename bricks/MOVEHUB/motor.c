
#include <stdbool.h>
#include <stdio.h>

#include "stm32f070xb.h"

#include "motor.h"

typedef struct {
    int32_t pos;
    uint16_t prev_timestamp;
} motor_tacho_data_t;

// only for ports A/B
static motor_tacho_data_t motor_tacho_data[2];

void motor_init(void) {
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
    SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI0_PA | SYSCFG_EXTICR1_EXTI1_PB;
    EXTI->IMR = EXTI_EMR_MR0 | EXTI_EMR_MR1;
    EXTI->RTSR = EXTI_RTSR_RT0 | EXTI_RTSR_RT1;
    EXTI->FTSR = EXTI_FTSR_FT0 | EXTI_FTSR_FT1;
    NVIC_EnableIRQ(EXTI0_1_IRQn);
    NVIC_SetPriority(EXTI0_1_IRQn, 0);

    // TIM7 is used for clock in speed measurement
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    TIM7->PSC = 479;    // divide 48MHz by 480 (= 479 + 1) to get 100kHz clock.
    TIM7->CR1 = TIM_CR1_CEN;
    TIM7->DIER = TIM_DIER_UIE;

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

static void motor_tacho_update_pos(motor_port_t port, bool int_pin_state, bool dir_pin_state, uint16_t timestamp) {
    // TODO: log timestamp for speed calculation
    if (dir_pin_state ^ int_pin_state) {
        motor_tacho_data[port].pos--;
    }
    else {
       motor_tacho_data[port].pos++;
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

    // clear events 0 an 1 (the tacho interupts)
    EXTI->PR = EXTI_PR_PR0 | EXTI_PR_PR1;

    // port A
    if (exti_pr & EXTI_PR_PR1) {
        gpio_idr = GPIOB->IDR;
        int_pin_state = !!(gpio_idr & GPIO_IDR_1);
        dir_pin_state = !!(gpio_idr & GPIO_IDR_9);
        motor_tacho_update_pos(MOTOR_PORT_A, int_pin_state, dir_pin_state, timestamp);
    }

    // port B
    if (exti_pr & EXTI_PR_PR0) {
        gpio_idr = GPIOA->IDR;
        int_pin_state = !!(gpio_idr & GPIO_IDR_0);
        dir_pin_state = !!(gpio_idr & GPIO_IDR_1);
        motor_tacho_update_pos(MOTOR_PORT_B, int_pin_state, dir_pin_state, timestamp);
    }
}

int motor_get_position(motor_port_t port, int *pos) {
    if (port < MOTOR_PORT_A || port > MOTOR_PORT_B) {
        return -1;
    }

    *pos = motor_tacho_data[port].pos;

    return 0;
}

static void motor_brake(motor_port_t port) {
    // set both port pins 1 and 2 to output high
    switch (port) {
    case MOTOR_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_8;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_10;
        break;
    case MOTOR_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_9;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_11;
        break;
    case MOTOR_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_6;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_8;
        break;
    case MOTOR_PORT_D:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_7;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_9;
        break;
    }
}

static void motor_coast(motor_port_t port) {
    // set both port pins 1 and 2 to output low
    switch (port) {
    case MOTOR_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOA->BRR = GPIO_BRR_BR_8;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
        GPIOA->BRR = GPIO_BRR_BR_10;
        break;
    case MOTOR_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOA->BRR = GPIO_BRR_BR_9;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
        GPIOA->BRR = GPIO_BRR_BR_11;
        break;
    case MOTOR_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
        GPIOC->BRR = GPIO_BRR_BR_6;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOC->BRR = GPIO_BRR_BR_8;
        break;
    case MOTOR_PORT_D:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
        GPIOC->BRR = GPIO_BRR_BR_7;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOC->BRR = GPIO_BRR_BR_9;
        break;
    }
}

static void motor_run_fwd(motor_port_t port, int duty_cycle) {
    // one pin as out, high and the other as PWM
    switch (port) {
    case MOTOR_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_8;
        TIM1->CCR3 = 10000 - duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (2 << GPIO_MODER_MODER10_Pos);
        break;
    case MOTOR_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_11;
        TIM1->CCR2 = 10000 - duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (2 << GPIO_MODER_MODER9_Pos);
        break;
    case MOTOR_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_6;
        TIM3->CCR3 = 10000 - duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
        break;
    case MOTOR_PORT_D:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_9;
        TIM3->CCR2 = 10000 - duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (2 << GPIO_MODER_MODER7_Pos);
        break;
    }
}

static void motor_run_rev(motor_port_t port, int duty_cycle) {
    // one pin as out, high and the other as PWM
    switch (port) {
    case MOTOR_PORT_A:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER10_Msk) | (1 << GPIO_MODER_MODER10_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_10;
        TIM1->CCR1 = 10000 + duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
        break;
    case MOTOR_PORT_B:
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER9_Msk) | (1 << GPIO_MODER_MODER9_Pos);
        GPIOA->BSRR = GPIO_BSRR_BS_9;
        TIM1->CCR4 = 10000 + duty_cycle;
        GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER11_Msk) | (2 << GPIO_MODER_MODER11_Pos);
        break;
    case MOTOR_PORT_C:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_8;
        TIM3->CCR1 = 10000 + duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER6_Msk) | (2 << GPIO_MODER_MODER6_Pos);
        break;
    case MOTOR_PORT_D:
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER7_Msk) | (1 << GPIO_MODER_MODER7_Pos);
        GPIOC->BSRR = GPIO_BSRR_BS_7;
        TIM3->CCR4 = 10000 + duty_cycle;
        GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER9_Msk) | (2 << GPIO_MODER_MODER9_Pos);
        break;
    }
}

int motor_run(motor_port_t port, int duty_cycle) {
    if (port < MOTOR_PORT_A || port > MOTOR_PORT_D) {
        return -1;
    }

    if (duty_cycle < -10000 || duty_cycle > 10000) {
        return -2;
    }

    if (duty_cycle > 0) {
        motor_run_fwd(port, duty_cycle);
    } else if (duty_cycle < 0) {
        motor_run_rev(port, duty_cycle);
    } else {
        motor_brake(port);
    }

    return 0;
}

int motor_stop(motor_port_t port, motor_stop_t stop_action) {
    if (port < MOTOR_PORT_A || port > MOTOR_PORT_D) {
        return -1;
    }

    switch (stop_action) {
    case MOTOR_STOP_COAST:
        motor_coast(port);
        break;
    case MOTOR_STOP_BRAKE:
        motor_brake(port);
        break;
    default:
        return -2;
    }

    return 0;
}

void motor_deinit(void) {
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
