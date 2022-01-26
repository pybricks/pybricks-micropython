// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

#ifndef _PB_CONVERSIONS_H_
#define _PB_CONVERSIONS_H_

/**
 * LEGO Color IDs.
 *
 * These apply to at least the 3x3 Light Matrix and possibly other devices.
 */
typedef enum {
    PB_PUP_COLOR_ID_NONE = 0,
    PB_PUP_COLOR_ID_MAGENTA = 1,
    PB_PUP_COLOR_ID_VIOLET = 2,
    PB_PUP_COLOR_ID_BLUE = 3,
    PB_PUP_COLOR_ID_CYAN = 4,
    PB_PUP_COLOR_ID_SPRING_GREEN = 5,
    PB_PUP_COLOR_ID_GREEN = 6,
    PB_PUP_COLOR_ID_YELLOW = 7,
    PB_PUP_COLOR_ID_ORANGE = 8,
    PB_PUP_COLOR_ID_RED = 9,
    PB_PUP_COLOR_ID_WHITE = 10,
} pb_powered_up_color_id_t;

pb_powered_up_color_id_t pb_powered_up_color_id_from_hue(uint16_t hue);

#endif // _PB_CONVERSIONS_H_
