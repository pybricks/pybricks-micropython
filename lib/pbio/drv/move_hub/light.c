
#include <pbdrv/light.h>
#include <pbio/port.h>
#include <pbio/error.h>

#include "stm32f070xb.h"

// setup LED PWMs and pins
void pbdrv_light_init(void) {
    // RGB values are 0-255, so multiplying by 5 here to limit brightness to
    // 1/5 of max possible without having to do division later. It should also
    // give use smoother steps than the official LEGO firmware since we aren't
    // jumping by 5s.
    TIM15->PSC = 187;
    TIM15->ARR = 256 * 5;
    TIM15->BDTR |= TIM_BDTR_MOE;
    TIM16->PSC = 187;
    TIM16->ARR = 256 * 5;
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
    TIM15->CCR1 = 0;
    TIM15->CCR2 = 0;
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
// turn off the light
void pbdrv_light_deinit(void) {
    TIM15->CR1 &= ~TIM_CR1_CEN;
    TIM16->CR1 &= ~TIM_CR1_CEN;
    GPIOB->BRR = GPIO_BRR_BR_8;
    GPIOB->BRR = GPIO_BRR_BR_14;
    GPIOB->BRR = GPIO_BRR_BR_15;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER8_Msk) | (1 << GPIO_MODER_MODER8_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER14_Msk) | (1 << GPIO_MODER_MODER14_Pos);
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER15_Msk) | (1 << GPIO_MODER_MODER15_Pos);
}
#endif

static uint8_t pbdrv_light_red;
static uint8_t pbdrv_light_green;
static uint8_t pbdrv_light_blue;
static pbdrv_light_pattern_t pbdrv_light_pattern;

static void pbdrv_light_poke_hw(uint8_t r, uint8_t g, uint8_t b)
{
    TIM16->CCR1 = r;
    TIM15->CCR1 = g;
    TIM15->CCR2 = b;
}

pbio_error_t pbdrv_light_set_color(pbio_port_t port, uint8_t r, uint8_t g, uint8_t b) {
    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        // TODO: check for Powered UP Lights connected to ports C/D
        return PBIO_ERROR_NO_DEV;
    }

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbdrv_light_red = r;
    pbdrv_light_green = g;
    pbdrv_light_blue = b;

    pbdrv_light_set_pattern(port, pbdrv_light_pattern);

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_light_set_pattern(pbio_port_t port, pbdrv_light_pattern_t pattern) {
    if (port == PBIO_PORT_C || port == PBIO_PORT_D) {
        // TODO: check for Powered UP Lights connected to ports C/D
        return PBIO_ERROR_NO_DEV;
    }

    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    switch (pattern) {
    case PBDRV_LIGHT_PATTERN_OFF:
        pbdrv_light_poke_hw(0, 0, 0);
        break;
    case PBDRV_LIGHT_PATTERN_ON:
        pbdrv_light_poke_hw(pbdrv_light_red, pbdrv_light_green, pbdrv_light_blue);
        break;
    default:
        return PBIO_ERROR_INVALID_ARG;
    }

    return PBIO_SUCCESS;
}
