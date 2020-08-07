// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// PWM driver using TI TLC5955 LED driver connected to STM32 MCU via SPI.

#ifndef _PBDRV_PWM_TLC5955_STM32_H_
#define _PBDRV_PWM_TLC5955_STM32_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TLC5955_STM32

#include <stdint.h>

#include STM32_H

#include <pbdrv/pwm.h>

/** Platform-specific device information. */
typedef struct {
    /** The SPI peripheral to use. */
    SPI_TypeDef *SPIx;
    /** LAT signal GPIO bank. */
    GPIO_TypeDef *lat_gpio;
    /** LAT signal GPIO pin. */
    uint16_t lat_gpio_pin;
    /** Unique ID (array index) for this instance. */
    uint8_t id;
} pbdrv_pwm_tlc5955_stm32_platform_data_t;

// Defined in platform.c
extern const pbdrv_pwm_tlc5955_stm32_platform_data_t
    pbdrv_pwm_tlc5955_stm32_platform_data[PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV];

void pbdrv_pwm_tlc5955_stm32_init(pbdrv_pwm_dev_t *dev);

void pbdrv_pwm_tlc5955_irq(uint8_t index);

#else // PBDRV_CONFIG_PWM_TLC5955_STM32

#define pbdrv_pwm_tlc5955_stm32_init(dev)

#endif // PBDRV_CONFIG_PWM_TLC5955_STM32

#endif // _PBDRV_PWM_TLC5955_STM32_H_
