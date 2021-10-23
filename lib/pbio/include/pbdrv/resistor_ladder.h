// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup ResistorLadderDriver Driver: Resistor Ladder
 * @{
 */

#ifndef _PBDRV_RESISTOR_LADDER_H_
#define _PBDRV_RESISTOR_LADDER_H_

#include <stdint.h>

#include <pbio/error.h>

/** Resistor ladder input channel flags. */
typedef enum {
    /** Input channel 0. */
    PBDRV_RESISTOR_LADDER_CH_0 = 1 << 0,
    /** Input channel 1. */
    PBDRV_RESISTOR_LADDER_CH_1 = 1 << 1,
    /** Input channel 0. */
    PBDRV_RESISTOR_LADDER_CH_2 = 1 << 2,
} pbdrv_resistor_ladder_ch_flags_t;

pbio_error_t pbdrv_resistor_ladder_get(uint8_t id, pbdrv_resistor_ladder_ch_flags_t *flags);

#endif // _PBDRV_RESISTOR_LADDER_H_

/** @} */
