
#include <stdbool.h>

#include "stm32f070xb.h"

void button_init(void)
{
    // set PC13 input with pull up
    GPIOC->PUPDR = (GPIOC->PUPDR & ~GPIO_PUPDR_PUPDR13_Msk) | (1 << GPIO_PUPDR_PUPDR13_Pos);
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER13_Msk) | (0 << GPIO_MODER_MODER13_Pos);
}

bool button_get_state(void) {
    // PC13 is low when the button is pressed
    return !(GPIOC->IDR & GPIO_IDR_13);
}

void button_deinit(void)
{
}
