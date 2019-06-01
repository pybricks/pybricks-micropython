// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

/**
 * \addtogroup GPIODriver GPIO driver
 * @{
 */

#ifndef _PBDRV_GPIO_H_
#define _PBDRV_GPIO_H_

#include <stdint.h>

#include <pbdrv/config.h>

typedef struct {
    void *bank;
    const uint8_t pin;
} pbdrv_gpio_t;

#if PBDRV_CONFIG_GPIO

void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio);
void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio);
uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio);
void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt);

#else //

static inline void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio) { }
static inline void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio) { }
static inline uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio) { return 0; }
static inline void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) { }

#endif // PBDRV_CONFIG_GPIO

#endif // _PBDRV_GPIO_H_

/** @} */
