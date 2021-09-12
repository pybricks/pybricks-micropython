// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#include <pybricks/util_pb/pb_conversions.h>

uint8_t pb_powered_up_color_id_from_hue(uint8_t hue) {
    if (hue < 15) {
        return 9; // Red
    }
    if (hue < 45) {
        return 8; // Orange
    }
    if (hue < 90) {
        return 7; // Yellow
    }
    if (hue < 135) {
        return 6; // Green
    }
    if (hue < 165) {
        return 5; // Turquoise
    }
    if (hue < 210) {
        return 4; // Cyan
    }
    if (hue < 255) {
        return 3; // Blue
    }
    if (hue < 285) {
        return 2; // Violet
    }
    if (hue < 330) {
        return 1; // Magenta
    }
    return 9; // Red
}
