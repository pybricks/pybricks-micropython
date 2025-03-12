// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#ifndef _INTERNAL_GPIO_TIAM1808_H_
#define _INTERNAL_GPIO_TIAM1808_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>

typedef struct {
    uint32_t mux_id;
    uint32_t mux_mask;
    uint32_t mux_shift;
    uint32_t gpio_bank_id;
    uint32_t gpio_mode;
} pbdrv_gpio_ev3_mux_t;

/**
 * Helper initializer for pbdrv_gpio_t struct for EV3.
 *
 * The TI AM1808 starterware library has highly redundant definitions for pin
 * setup, so we use this macro to avoid small mistakes.
 *
 * @param MUX The mux ID.
 * @param HIGH The high end of the mux mask.
 * @param LOW The low end of the mux mask.
 * @param BANK The GPIO bank ID (zero-based).
 * @param PIN The GPIO pin ID (zero-based).
 */
#define PBDRV_GPIO_EV3_PIN(MUX, HIGH, LOW, BANK, PIN) \
    { \
        .bank = &(pbdrv_gpio_ev3_mux_t) { \
            .mux_id = MUX, \
            .mux_mask = SYSCFG_PINMUX##MUX##_PINMUX##MUX##_##HIGH##_##LOW, \
            .mux_shift = SYSCFG_PINMUX##MUX##_PINMUX##MUX##_##HIGH##_##LOW##_SHIFT, \
            .gpio_bank_id = BANK, \
            .gpio_mode = SYSCFG_PINMUX##MUX##_PINMUX##MUX##_##HIGH##_##LOW##_GPIO##BANK##_##PIN, \
        }, \
        .pin = PIN, \
    } \

#endif // _INTERNAL_GPIO_TIAM1808_H_
