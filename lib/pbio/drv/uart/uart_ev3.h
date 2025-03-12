// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// UART driver for STM32F4x using IRQ.

#ifndef _INTERNAL_PBDRV_UART_EV3_H_
#define _INTERNAL_PBDRV_UART_EV3_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <pbdrv/gpio.h>

/** Platform-specific information for UART peripheral. */
typedef struct {
    /** The UART base register address. */
    uint32_t base_address;
    /** PSC peripheral ID */
    uint32_t psc_peripheral_id;
    /** The UART interrupt number. */
    uint32_t sys_int_uart_int_id;
    /** TX Pin */
    pbdrv_gpio_t pin_tx;
    /** TX Pin Mux */
    uint32_t pin_tx_mux;
    /** RX Pin */
    pbdrv_gpio_t pin_rx;
    /** RX Pin Mux */
    uint32_t pin_rx_mux;
    /** Interrupt handler for this peripheral. */
    void (*isr_handler)(void);
} pbdrv_uart_ev3_platform_data_t;

/**
 * Array of UART platform data to be defined in platform.c.
 */
extern const pbdrv_uart_ev3_platform_data_t
    pbdrv_uart_ev3_platform_data[PBDRV_CONFIG_UART_EV3_NUM_UART];

/**
 * Callback to be called by the UART IRQ handler.
 * @param id [in]   The UART instance ID.
 */
void pbdrv_uart_ev3_handle_irq(uint8_t id);

#endif // _INTERNAL_PBDRV_UART_EV3_H_
