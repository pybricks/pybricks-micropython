// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

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
    SPI_TypeDef *spi;
    /** Interrupt number for SPI IRQ. */
    IRQn_Type spi_irq;
    #if defined(STM32H5)
    /** The SPI Rx DMA channel to use. */
    DMA_Channel_TypeDef *rx_dma;
    /** Request number for Rx DMA. */
    uint32_t rx_dma_req;
    /** The SPI Tx DMA channel to use. */
    DMA_Channel_TypeDef *tx_dma;
    /** Request number for Tx DMA. */
    uint32_t tx_dma_req;
    #else
    /** The SPI Rx DMA peripheral to use. */
    DMA_Stream_TypeDef *rx_dma;
    /** Channel number for Rx DMA. */
    uint32_t rx_dma_ch;
    /** The SPI Tx DMA peripheral to use. */
    DMA_Stream_TypeDef *tx_dma;
    /** Channel number for Tx DMA. */
    uint32_t tx_dma_ch;
    #endif
    /** Interrupt number for Rx DMA IRQ. */
    IRQn_Type rx_dma_irq;
    /** Interrupt number for Tx DMA IRQ. */
    IRQn_Type tx_dma_irq;
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

void pbdrv_pwm_tlc5955_stm32_rx_dma_irq(uint8_t index);
void pbdrv_pwm_tlc5955_stm32_tx_dma_irq(uint8_t index);
void pbdrv_pwm_tlc5955_stm32_spi_irq(uint8_t index);
void pbdrv_pwm_tlc5955_stm32_spi_tx_complete(void);

#else // PBDRV_CONFIG_PWM_TLC5955_STM32

#define pbdrv_pwm_tlc5955_stm32_init(dev)

#endif // PBDRV_CONFIG_PWM_TLC5955_STM32

#endif // _PBDRV_PWM_TLC5955_STM32_H_
