// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for W25Qxx.

#ifndef _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_STM32_H_
#define _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_STM32_H_

#include <stdint.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32

#include <contiki.h>

#include <pbdrv/gpio.h>

#include <pbio/error.h>

#include STM32_HAL_H

/** Platform-specific information for w25qxx flash storage. */
typedef struct {
    /** The SPI Tx DMA peripheral. */
    DMA_Stream_TypeDef *tx_dma;
    /** The SPI Tx DMA channel. */
    uint32_t tx_dma_ch;
    /** The SPI Tx DMA interrupt number. */
    IRQn_Type tx_dma_irq;
    /** The SPI Rx DMA peripheral */
    DMA_Stream_TypeDef *rx_dma;
    /** The SPI Rx DMA channel. */
    uint32_t rx_dma_ch;
    /** The SPI Rx DMA interrupt number. */
    IRQn_Type rx_dma_irq;
    /** The SPI registers. */
    SPI_TypeDef *spi;
    /** The SPI interrupt number. */
    IRQn_Type irq;
    /** The /CS pin. */
    const pbdrv_gpio_t pin_ncs;
} pbdrv_block_device_w25qxx_stm32_platform_data_t;

/**
 * Block device platform data to be defined in platform.c.
 */
extern const pbdrv_block_device_w25qxx_stm32_platform_data_t pbdrv_block_device_w25qxx_stm32_platform_data;

void pbdrv_block_device_w25qxx_stm32_spi_irq(void);
void pbdrv_block_device_w25qxx_stm32_spi_tx_complete(void);
void pbdrv_block_device_w25qxx_stm32_spi_rx_complete(void);
void pbdrv_block_device_w25qxx_stm32_spi_error(void);
void pbdrv_block_device_w25qxx_stm32_spi_handle_tx_dma_irq(void);
void pbdrv_block_device_w25qxx_stm32_spi_handle_rx_dma_irq(void);

#endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32

#endif // _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_STM32_H_
