// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PYBRICKS_MOVEHUB_GPIO_H_
#define _PYBRICKS_MOVEHUB_GPIO_H_

#include "stm32f030xc.h"

// simple GPIO interface
#define GPIO_MODE_IN (0)
#define GPIO_MODE_OUT (1)
#define GPIO_MODE_ALT (2)
#define GPIO_PULL_NONE (0)
#define GPIO_PULL_UP (0)
#define GPIO_PULL_DOWN (1)

static inline void gpio_init(GPIO_TypeDef *gpio, int pin, int mode, int pull, int alt) {
    gpio->MODER = (gpio->MODER & ~(3 << (2 * pin))) | (mode << (2 * pin));
    // OTYPER is left as default push-pull
    // OSPEEDR is left as default low speed
    gpio->PUPDR = (gpio->PUPDR & ~(3 << (2 * pin))) | (pull << (2 * pin));
    gpio->AFR[pin >> 3] = (gpio->AFR[pin >> 3] & ~(15 << (4 * (pin & 7)))) | (alt << (4 * (pin & 7)));
}

static inline uint8_t gpio_get(GPIO_TypeDef *gpio, uint8_t pin) {
    gpio->MODER &= ~(3 << (2 * pin));
    return (gpio->IDR >> pin) & 1;
}

static inline void gpio_low(GPIO_TypeDef *gpio, uint8_t pin) {
    gpio->BRR = (1 << (pin));
    gpio->MODER = (gpio->MODER & ~(3 << (2 * pin))) | (1 << (2 * pin));
}

static inline void gpio_high(GPIO_TypeDef *gpio, uint8_t pin) {
    gpio->BSRR = (1 << (pin));
    gpio->MODER = (gpio->MODER & ~(3 << (2 * pin))) | (1 << (2 * pin));
}

#endif /* _PYBRICKS_MOVEHUB_GPIO_H_ */
