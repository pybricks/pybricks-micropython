
#include "stm32f070xb.h"

// setup LED PWMs and pins
void led_init(void) {
    TIM15->PSC = 187;
    TIM15->ARR = 255;
    TIM15->BDTR |= TIM_BDTR_MOE;
    TIM16->PSC = 187;
    TIM16->ARR = 255;
    TIM16->BDTR |= TIM_BDTR_MOE;
    
    // red LED on PB8 using TIM16 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER8_Msk) | (2 << GPIO_MODER_MODER8_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL8_Msk) | (2 << GPIO_AFRH_AFSEL8_Pos);
    TIM16->CCMR1 |= (0x6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1
    TIM16->CCER |= TIM_CCER_CC1E;

    // green LED on PB14 using TIM15 CH1
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (2 << GPIO_MODER_MODER14_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL14_Msk) | (1 << GPIO_AFRH_AFSEL14_Pos);
    TIM15->CCMR1 |= (0x6 << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE; // PWM Mode 1
    TIM15->CCER |= TIM_CCER_CC1E;

    // blue LED on PB15 using TIM15 CH2
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER15_Msk) | (2 << GPIO_MODER_MODER15_Pos);
    GPIOB->AFR[1] = (GPIOB->AFR[1] & ~GPIO_AFRH_AFSEL15_Msk) | (1 << GPIO_AFRH_AFSEL15_Pos);
    TIM15->CCMR1 |= (0x6 << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE; // PWM Mode 1
    TIM15->CCER |= TIM_CCER_CC2E;

    TIM16->CR1 |= TIM_CR1_CEN;
    TIM16->EGR |= TIM_EGR_UG;
    TIM15->CR1 |= TIM_CR1_CEN;
    TIM15->EGR |= TIM_EGR_UG;
    TIM16->CCR1 = 0;
    TIM15->CCR1 = 51;
    TIM15->CCR2 = 0;
}

void led_set_rgb(int r, int g, int b) {
    // TODO: need to check for valid range
    // div round up by 5 comes from LEGO firmware
    TIM16->CCR1 = (r + 4) / 5;
    TIM15->CCR1 = (g + 4) / 5;
    TIM15->CCR2 = (b + 4) / 5;
}

// turn off the light
void led_deinit(void) {
    TIM15->CR1 &= ~TIM_CR1_CEN;
    TIM16->CR1 &= ~TIM_CR1_CEN;
    GPIOB->BRR = GPIO_BRR_BR_8;
    GPIOB->BRR = GPIO_BRR_BR_14;
    GPIOB->BRR = GPIO_BRR_BR_15;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (1 << GPIO_MODER_MODER14_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER15_Msk) | (1 << GPIO_MODER_MODER15_Pos);
}
