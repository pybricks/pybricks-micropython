// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// PWM driver using TI LP50XX LED driver connected to STM32 MCU via FMPI2C.

#ifndef _PBDRV_PWM_LP50XX_STM32_H_
#define _PBDRV_PWM_LP50XX_STM32_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_LP50XX_STM32

#include <stdint.h>

#include STM32_H

#include <pbdrv/pwm.h>

/** Platform-specific device information. */
typedef struct {
    /** The I2C peripheral to use. */
    FMPI2C_TypeDef *i2c;
    /** Interrupt number for I2C EV IRQ. */
    IRQn_Type i2c_ev_irq;
    /** Interrupt number for I2C ER IRQ. */
    IRQn_Type i2c_er_irq;
    /** The I2C Rx DMA peripheral to use. */
    DMA_Stream_TypeDef *rx_dma;
    /** Channel number for Rx DMA. */
    uint32_t rx_dma_ch;
    /** Interrupt number for Rx DMA IRQ. */
    IRQn_Type rx_dma_irq;
    /** The I2C Tx DMA peripheral to use. */
    DMA_Stream_TypeDef *tx_dma;
    /** Channel number for Tx DMA. */
    uint32_t tx_dma_ch;
    /** Interrupt number for Tx DMA IRQ. */
    IRQn_Type tx_dma_irq;
    /** Enable signal GPIO bank. */
    GPIO_TypeDef *en_gpio;
    /** Enable signal GPIO pin. */
    uint16_t en_gpio_pin;
    /** Unique ID (array index) for this instance. */
    uint8_t id;
} pbdrv_pwm_lp50xx_stm32_platform_data_t;

// Defined in platform.c
extern const pbdrv_pwm_lp50xx_stm32_platform_data_t
    pbdrv_pwm_lp50xx_stm32_platform_data[PBDRV_CONFIG_PWM_LP50XX_STM32_NUM_DEV];

void pbdrv_pwm_lp50xx_stm32_init(pbdrv_pwm_dev_t *dev);

void pbdrv_pwm_lp50xx_stm32_rx_dma_irq(uint8_t index);
void pbdrv_pwm_lp50xx_stm32_tx_dma_irq(uint8_t index);
void pbdrv_pwm_lp50xx_stm32_i2c_ev_irq(uint8_t index);
void pbdrv_pwm_lp50xx_stm32_i2c_er_irq(uint8_t index);

#else // PBDRV_CONFIG_PWM_LP50XX_STM32

#define pbdrv_pwm_lp50xx_stm32_init(dev)

#endif // PBDRV_CONFIG_PWM_LP50XX_STM32

#endif // _PBDRV_PWM_LP50XX_STM32_H_
