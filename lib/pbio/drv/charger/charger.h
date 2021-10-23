// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_CHARGER_H_
#define _INTERNAL_PBDRV_CHARGER_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CHARGER

void pbdrv_charger_init(void);

#else // PBDRV_CONFIG_CHARGER

#define pbdrv_charger_init()

#endif // PBDRV_CONFIG_CHARGER

#endif // _INTERNAL_PBDRV_CHARGER_H_
