// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Simple resistor ladder driver

// SPIKE Prime uses analog inputs to multiplex multiple digital inputs using
// a simple resistor ladder as shown below:
//
//  ^ 3.3V
//  |
//  Z 10k
//  |
//  +-------+-------+----> AIN
//  |       |       |
//    [ A     [ B     [ C
//  |       |       |
//  Z 18k   Z 33k   Z 82k
//  |       |       |
//  +-------+-------+
//  V GND

#include <pbdrv/config.h>

#if PBDRV_CONFIG_RESISTOR_LADDER

#include <pbdrv/adc.h>
#include <pbdrv/resistor_ladder.h>
#include <pbio/error.h>

#include "resistor_ladder.h"

/**
 * Gets bit flags indicating the state of each input.
 * @param [in]  id          The resistor ladder device ID.
 * @param [out] flags       The flags.
 * @return                  ::PBIO_SUCCESS, ::PBIO_ERROR_NO_DEV if @p id is not
 *                          valid or error passed from ADC.
 */
pbio_error_t pbdrv_resistor_ladder_get(uint8_t id, pbdrv_resistor_ladder_ch_flags_t *flags) {
    if (id >= PBDRV_CONFIG_RESISTOR_LADDER_NUM_DEV) {
        return PBIO_ERROR_NO_DEV;
    }

    const pbdrv_resistor_ladder_platform_data_t *pdata = &pbdrv_resistor_ladder_platform_data[id];

    uint16_t value;
    pbio_error_t err = pbdrv_adc_get_ch(pdata->adc_ch, &value);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (value > pdata->level[0]) {
        *flags = 0;
    } else if (value > pdata->level[1]) {
        *flags = PBDRV_RESISTOR_LADDER_CH_2;
    } else if (value > pdata->level[2]) {
        *flags = PBDRV_RESISTOR_LADDER_CH_1;
    } else if (value > pdata->level[3]) {
        *flags = PBDRV_RESISTOR_LADDER_CH_1 | PBDRV_RESISTOR_LADDER_CH_2;
    } else if (value > pdata->level[4]) {
        *flags = PBDRV_RESISTOR_LADDER_CH_0;
    } else if (value > pdata->level[5]) {
        *flags = PBDRV_RESISTOR_LADDER_CH_0 | PBDRV_RESISTOR_LADDER_CH_2;
    } else if (value > pdata->level[6]) {
        *flags = PBDRV_RESISTOR_LADDER_CH_0 | PBDRV_RESISTOR_LADDER_CH_1;
    } else if (value > pdata->level[7]) {
        *flags = PBDRV_RESISTOR_LADDER_CH_0 | PBDRV_RESISTOR_LADDER_CH_1 | PBDRV_RESISTOR_LADDER_CH_2;
    } else {
        // hardware failure?
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_RESISTOR_LADDER
