// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Internal functions for LEGO UART I/O devices.

#ifndef _PBIO_UARTDEV_H_
#define _PBIO_UARTDEV_H_

#include <pbio/config.h>

#if PBIO_CONFIG_UARTDEV

#include <pbdrv/counter.h>

void pbio_uartdev_counter_init(pbdrv_counter_dev_t *devs);

#else // PBIO_CONFIG_UARTDEV

#define pbio_uartdev_counter_init(devs)

#endif // PBIO_CONFIG_UARTDEV

#endif // _PBIO_UARTDEV_H_
