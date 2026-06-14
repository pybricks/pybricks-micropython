// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 The Pybricks Authors

// Block device driver for W25Qxx.

#ifndef _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_STM32_H_
#define _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_STM32_H_

#include <stdint.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32

#include <pbio/error.h>

#include STM32_HAL_H

#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

/** STM32H5 XSPI platform information for w25qxx flash storage. */
typedef struct {
    /** The OCTOSPI1 instance. */
    OCTOSPI_TypeDef *octospi;
    /** The XSPI clock prescaler (fXSPI_CLK = fKERNEL / (prescaler + 1)). */
    uint32_t clock_prescaler;
    /** The XSPI Tx DMA channel. */
    DMA_Channel_TypeDef *tx_dma;
    /** The XSPI Tx DMA request. */
    uint32_t tx_dma_req;
    /** The XSPI Rx DMA channel */
    DMA_Channel_TypeDef *rx_dma;
    /** The XSPI Rx DMA request. */
    uint32_t rx_dma_req;
    /** The XSPI Tx DMA interrupt number. */
    IRQn_Type tx_dma_irq;
    /** The XSPI Rx DMA interrupt number. */
    IRQn_Type rx_dma_irq;
    /** The XSPI interrupt number. */
    IRQn_Type irq;
} pbdrv_block_device_w25qxx_stm32_platform_data_t;

#else // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

#include <pbdrv/gpio.h>

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

#endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

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
#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI
void pbdrv_block_device_w25qxx_stm32_spi_cmd_complete(void);
#endif

#endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32

#endif // _INTERNAL_PBDRV_BLOCK_DEVICE_W25QXX_STM32_H_
