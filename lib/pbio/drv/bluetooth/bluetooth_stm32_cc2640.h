// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PVDRV_BLUETOOTH_STM32_CC2640_H_
#define _INTERNAL_PVDRV_BLUETOOTH_STM32_CC2640_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>

#if PBDRV_CONFIG_BLUETOOTH_STM32_CC2640

typedef struct {
    /** Bluetooth address */
    const uint8_t *bd_addr;
    /** GPIO connected to nRESET pin. */
    pbdrv_gpio_t reset_gpio;
    /** GPIO connected to MRDY pin. */
    pbdrv_gpio_t mrdy_gpio;
    /** SPI initialization callback. */
    void (*spi_init)(void);
    /** SPI start transfer callback. */
    void (*spi_start_xfer)(const uint8_t *tx_buf, uint8_t *rx_buf, uint8_t xfer_size);
} pbdrv_bluetooth_stm32_cc2640_platform_data_t;

/** SRDY signal change interrupt handler. */
void pbdrv_bluetooth_stm32_cc2640_srdy_irq(bool srdy);

/** SPI transver complete interrupt handler. */
void pbdrv_bluetooth_stm32_cc2640_spi_xfer_irq(void);

/** Platform data (defined in platform.c) */
extern const pbdrv_bluetooth_stm32_cc2640_platform_data_t pbdrv_bluetooth_stm32_cc2640_platform_data;

#endif // PBDRV_CONFIG_BLUETOOTH_STM32_CC2640

#endif // _INTERNAL_PVDRV_BLUETOOTH_STM32_CC2640_H_
