// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_PUP_H_
#define _INTERNAL_PBDRV_IOPORT_PUP_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_PUP

#include <pbio/port.h>

#include <pbdrv/gpio.h>

typedef struct {
    pbdrv_gpio_t gpio1;      // pin 5 in
    pbdrv_gpio_t gpio2;      // pin 6 in/out
    pbdrv_gpio_t uart_buf;
    pbdrv_gpio_t uart_tx;    // pin 5 out
    pbdrv_gpio_t uart_rx;    // pin 6 in
    uint8_t uart_alt;
} pbdrv_ioport_pup_pins_t;

typedef struct {
    /** The I/O pins of the port. */
    const pbdrv_ioport_pup_pins_t pins;
    /** The I/O port identifier this port is associated with. */
    pbio_port_id_t port_id;
    /** Index of associated motor driver. */
    uint8_t motor_driver_index;
    /** Index of associated UART driver. */
    uint8_t uart_driver_index;
} pbdrv_ioport_pup_port_platform_data_t;

typedef struct {
    /** Supplies 3.3V power to pin 4 on all ports. */
    pbdrv_gpio_t port_vcc;
    /** Individual port data. */
    pbdrv_ioport_pup_port_platform_data_t ports[PBDRV_CONFIG_IOPORT_NUM_DEV];
} pbdrv_ioport_pup_platform_data_t;

extern const pbdrv_ioport_pup_platform_data_t pbdrv_ioport_pup_platform_data;

#if PBDRV_CONFIG_IOPORT_PUP_QUIRK_SHUTDOWN
// this function must be defined in platform.c when quirk is enabled
bool pbdrv_ioport_needs_shutdown_quirk(void);
#endif

#endif // PBDRV_CONFIG_IOPORT_PUP

#endif // _INTERNAL_PBDRV_IOPORT_PUP_H_
