// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_LPF2_H_
#define _INTERNAL_PBDRV_IOPORT_LPF2_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>

// GPIOs associated with ID1 and ID2 pins
typedef struct {
    pbdrv_gpio_t id1;        // pin 5 in
    pbdrv_gpio_t id2;        // pin 6 in/out
    pbdrv_gpio_t uart_buf;
    pbdrv_gpio_t uart_tx;    // pin 5 out
    pbdrv_gpio_t uart_rx;    // pin 6 in
    uint8_t alt;
} pbdrv_ioport_lpf2_port_platform_data_t;

typedef struct {
    /** Supplies 3.3V power to pin 4 on all ports. */
    pbdrv_gpio_t port_vcc;
    /** Individual port data. */
    pbdrv_ioport_lpf2_port_platform_data_t ports[PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS];
} pbdrv_ioport_lpf2_platform_data_t;

// platforom configuration checks
#ifndef PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS
#error Platform must define PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS
#endif
#ifndef PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT
#error Platform must define PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT
#endif
#ifndef PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT
#error Platform must define PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT
#endif

// This is defined in platform/*/platform.c
extern const pbdrv_ioport_lpf2_platform_data_t pbdrv_ioport_lpf2_platform_data;

#endif // _INTERNAL_PBDRV_IOPORT_LPF2_H_
