// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RESISTOR_LADDER_H_
#define _INTERNAL_PBDRV_RESISTOR_LADDER_H_

#include <stdint.h>

#include <pbdrv/config.h>

typedef struct {
    /** The AD/C raw values to delinite the cutoff levels for each digital input. */
    uint16_t level[8];
    /** The AD/C channel connected to this resistor ladder. */
    uint8_t adc_ch;
} pbdrv_resistor_ladder_platform_data_t;

extern const pbdrv_resistor_ladder_platform_data_t pbdrv_resistor_ladder_platform_data[PBDRV_CONFIG_RESISTOR_LADDER_NUM_DEV];

#endif // _INTERNAL_PBDRV_RESISTOR_LADDER_H_
