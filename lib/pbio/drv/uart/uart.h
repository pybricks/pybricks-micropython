// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Common interface shared by UART drivers

#ifndef _INTERNAL_PBDRV_UART_H_
#define _INTERNAL_PBDRV_UART_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART

/**
 * Initializes the UART driver.
 */
void pbdrv_uart_init(void);

#else // PBDRV_CONFIG_UART

#define pbdrv_uart_init()

#endif // PBDRV_CONFIG_UART

#endif // _INTERNAL_PBDRV_UART_H_
