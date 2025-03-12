// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// UART driver for STM32F4x using IRQ.

#ifndef _INTERNAL_PBDRV_UART_EV3_H_
#define _INTERNAL_PBDRV_UART_EV3_H_

#include <stdint.h>

#include "pbdrvconfig.h"

#include <pbdrv/gpio.h>

typedef enum {
    /** Hardware UART. */
    EV3_UART_HW,
    /** PRU Software UART. */
    EV3_UART_PRU,
} pbdrv_uart_ev3_uart_kind_t;

/** Platform-specific information for UART peripheral. */
typedef struct {
    /** UART Kind. */
    pbdrv_uart_ev3_uart_kind_t uart_kind;
    /** The UART base register address (only applicable in hardware mode). */
    uint32_t base_address;
    /** Peripheral ID (PSC id in case of hardware, PRU UART line ID in case of PRU) */
    uint32_t peripheral_id;
    /** The UART interrupt number. */
    uint32_t sys_int_uart_int_id;
    /** Interrupt handler for this peripheral. */
    void (*isr_handler)(void);
} pbdrv_uart_ev3_platform_data_t;

/**
 * Array of UART platform data to be defined in platform.c.
 */
extern const pbdrv_uart_ev3_platform_data_t
    pbdrv_uart_ev3_platform_data[PBDRV_CONFIG_UART_EV3_NUM_UART];

/**
 * Callback to be called by the hardware UART IRQ handler.
 *
 * The driver will forward this call to the relevant hardware or PRU UART
 * IRQ handler.
 *
 * @param id [in]   The UART instance ID.
 */
void pbdrv_uart_ev3_handle_irq(uint8_t id);

#endif // _INTERNAL_PBDRV_UART_EV3_H_
