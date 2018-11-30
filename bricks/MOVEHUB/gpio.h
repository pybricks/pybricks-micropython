/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _PYBRICKS_MOVEHUB_GPIO_H_
#define _PYBRICKS_MOVEHUB_GPIO_H_

#include "stm32f070xb.h"

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
