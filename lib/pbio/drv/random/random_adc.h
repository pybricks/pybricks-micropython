// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

// Random number generator using analog/digital convert as source of entropy.

#ifndef _INTERNAL_PBDRV_RANDOM_ADC_H_
#define _INTERNAL_PBDRV_RANDOM_ADC_H_

#include <stdint.h>

void pbdrv_random_adc_push_lsb(uint16_t value);

#endif // _INTERNAL_PBDRV_RANDOM_ADC_H_
