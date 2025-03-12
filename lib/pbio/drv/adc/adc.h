// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

// Internal common adc functions.

#ifndef _INTERNAL_PBDRV_ADC_H_
#define _INTERNAL_PBDRV_ADC_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_ADC

void pbdrv_adc_init(void);

#else // PBDRV_CONFIG_ADC

#define pbdrv_adc_init()

#endif // PBDRV_CONFIG_ADC

#endif // _INTERNAL_PBDRV_ADC_H_
