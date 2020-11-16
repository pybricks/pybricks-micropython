// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_LPF2_H_
#define _INTERNAL_PBDRV_IOPORT_LPF2_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/gpio.h>

// GPIOs associated with ID1 and ID2 pins
typedef struct {
    const pbdrv_gpio_t id1;        // pin 5 in
    const pbdrv_gpio_t id2;        // pin 6 in/out
    const pbdrv_gpio_t uart_buf;
    const pbdrv_gpio_t uart_tx;    // pin 5 out
    const pbdrv_gpio_t uart_rx;    // pin 6 in
    const uint8_t alt;
} pbdrv_ioport_lpf2_platform_port_t;

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

// These are defined in platform/*/platform.c
#if PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS > 0
extern const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_0;
#endif
#if PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS > 1
extern const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_1;
#endif
#if PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS > 2
extern const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_2;
#endif
#if PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS > 3
extern const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_3;
#endif
#if PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS > 4
extern const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_4;
#endif
#if PBDRV_CONFIG_IOPORT_LPF2_NUM_PORTS > 5
extern const pbdrv_ioport_lpf2_platform_port_t pbdrv_ioport_lpf2_platform_port_5;
#endif

#endif // _INTERNAL_PBDRV_IOPORT_LPF2_H_
