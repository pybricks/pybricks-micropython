// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_CHARGER_H_
#define _INTERNAL_PBDRV_CHARGER_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CHARGER

#include <pbdrv/charger.h>

void pbdrv_charger_init(void);

void pbdrv_charger_set_usb_type(pbdrv_charger_usb_type_t type);

#else // PBDRV_CONFIG_CHARGER

#define pbdrv_charger_init()
#define pbdrv_charger_set_usb_type(type)

#endif // PBDRV_CONFIG_CHARGER

#endif // _INTERNAL_PBDRV_CHARGER_H_
