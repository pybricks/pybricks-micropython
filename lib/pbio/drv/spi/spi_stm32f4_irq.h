// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

// SPI driver for STM32F4x using IRQ.

#ifndef _INTERNAL_PBDRV_SPI_STM32F4_IRQ_H_
#define _INTERNAL_PBDRV_SPI_STM32F4_IRQ_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <stm32f4xx.h>

#include <stm32f4xx_hal_spi.h>

/** Platform-specific information for SPI peripheral. */
typedef struct {
    /** The SPI handle. */
    SPI_HandleTypeDef *hspi;
    /** The SPI registers. */
    SPI_TypeDef *spi;
    /** The SPI interupt number. */
    IRQn_Type irq;
    /** The /CS pin. */
    const pbdrv_gpio_t pin_ncs;
} pbdrv_spi_stm32f4_irq_platform_data_t;

/**
 * Array of SPI platform data to be defined in platform.c.
 */
extern const pbdrv_spi_stm32f4_irq_platform_data_t
    pbdrv_spi_stm32f4_irq_platform_data[PBDRV_CONFIG_SPI_STM32F4_IRQ_NUM_SPI];

/**
 * Callback on transfer completion.
 * @param id [in]   The SPI instance ID.
 */
void pbdrv_spi_stm32f4_irq_handle_txrx_complete(uint8_t id);

/**
 * Callback on transmission error.
 * @param id [in]   The SPI instance ID.
 */
void pbdrv_spi_stm32f4_irq_handle_error(uint8_t id);

#endif // _INTERNAL_PBDRV_SPI_STM32F4_IRQ_H_
