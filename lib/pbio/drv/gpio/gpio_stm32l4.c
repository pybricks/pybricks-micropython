
#include "pbdrv/config.h"

#if PBDRV_CONFIG_GPIO_STM32L4

#include <stdint.h>

#include "pbdrv/gpio.h"

#include "stm32l4xx.h"

void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio) {
    GPIO_TypeDef *bank = gpio->bank;
    bank->BSRR = (1 << 16) << gpio->pin;
    bank->MODER = (bank->MODER & ~(3 << (gpio->pin * 2))) | (1 << (gpio->pin * 2));
}

void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio) {
    GPIO_TypeDef *bank = gpio->bank;
    bank->BSRR = 1 << gpio->pin;
    bank->MODER = (bank->MODER & ~(3 << (gpio->pin * 2))) | (1 << (gpio->pin * 2));
}

uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio) {
    GPIO_TypeDef *bank = gpio->bank;
    bank->MODER = (bank->MODER & ~(3 << (gpio->pin * 2))) | (0 << (gpio->pin * 2));
    return (bank->IDR >> gpio->pin) & 1;
}

void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) {
    GPIO_TypeDef *bank = gpio->bank;
    bank->AFR[gpio->pin / 8] = (bank->AFR[gpio->pin / 8] & ~(0xf << (gpio->pin % 8 * 4))) | (alt << (gpio->pin % 8 * 4));
    bank->MODER = (bank->MODER & ~(3 << (gpio->pin * 2))) | (2 << (gpio->pin * 2));
}

void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull) {
    GPIO_TypeDef *bank = gpio->bank;
    bank->PUPDR = (bank->PUPDR & ~(3 << (gpio->pin * 2))) | (pull << (gpio->pin * 2));
}

#endif // PBDRV_CONFIG_GPIO_STM32L4
