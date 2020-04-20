// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * \addtogroup GPIODriver GPIO driver
 * @{
 */

#ifndef _PBDRV_GPIO_H_
#define _PBDRV_GPIO_H_

#include <stdint.h>

#include <pbdrv/config.h>

typedef enum {
    // NOTE: these values are picked to be the same as STM32 so that we don't
    // need a mapping function.
    PBDRV_GPIO_PULL_NONE,
    PBDRV_GPIO_PULL_UP,
    PBDRV_GPIO_PULL_DOWN,
} pbdrv_gpio_pull_t;

typedef struct {
    void *bank;
    const uint8_t pin;
} pbdrv_gpio_t;

#if PBDRV_CONFIG_GPIO

void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio);
void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio);
uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio);
void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt);
void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull);

#else //

static inline void pbdrv_gpio_out_low(const pbdrv_gpio_t *gpio) { }
static inline void pbdrv_gpio_out_high(const pbdrv_gpio_t *gpio) { }
static inline uint8_t pbdrv_gpio_input(const pbdrv_gpio_t *gpio) { return 0; }
static inline void pbdrv_gpio_alt(const pbdrv_gpio_t *gpio, uint8_t alt) { }
static inline void pbdrv_gpio_set_pull(const pbdrv_gpio_t *gpio, pbdrv_gpio_pull_t pull) { }

#endif // PBDRV_CONFIG_GPIO

#endif // _PBDRV_GPIO_H_

/** @} */
