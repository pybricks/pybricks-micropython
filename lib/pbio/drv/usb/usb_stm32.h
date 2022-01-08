// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Internal header for STM32 USB driver.

#ifndef _INTERNAL_PBDRV_USB_STM32_H_
#define _INTERNAL_PBDRV_USB_STM32_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_STM32F4

#include <stdbool.h>

void pbdrv_usb_stm32_handle_otg_fs_irq(void);

#endif // PBDRV_CONFIG_USB_STM32F4

#endif // _INTERNAL_PBDRV_USB_STM32_H_
