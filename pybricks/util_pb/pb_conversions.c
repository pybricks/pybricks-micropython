// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

#include <pbio/color.h>

#include "py/mpconfig.h"

#include <pybricks/util_pb/pb_conversions.h>

/**
 * Converts a hue to the closest matching LEGO color ID number used in Powered
 * Up devices.
 *
 * Since this only looks at hue, it will not return ::PB_PUP_COLOR_ID_WHITE or ::PB_PUP_COLOR_ID_NONE.
 *
 * @param [in]  hue     The hue in the range 0-359.
 * @return              The ID in the range ::PB_PUP_COLOR_ID_MAGENTA to ::PB_PUP_COLOR_ID_RED.
 */
pb_powered_up_color_id_t pb_powered_up_color_id_from_hue(uint16_t hue) {
    // Color divisions are 1/2 way between the matching hue and the next hue
    // of the LEGO color codes. For example, a hue of 13 will be considered
    // red since it is less than 1/2 way between red (0) and orange (30), but
    // a hue of 16 will be considered orange.

    if (hue < (PBIO_COLOR_HUE_RED + PBIO_COLOR_HUE_ORANGE) / 2) {
        return PB_PUP_COLOR_ID_RED;
    }
    if (hue < (PBIO_COLOR_HUE_ORANGE + PBIO_COLOR_HUE_YELLOW) / 2) {
        return PB_PUP_COLOR_ID_ORANGE;
    }
    if (hue < (PBIO_COLOR_HUE_YELLOW + PBIO_COLOR_HUE_GREEN) / 2) {
        return PB_PUP_COLOR_ID_YELLOW;
    }
    if (hue < (PBIO_COLOR_HUE_GREEN + PBIO_COLOR_HUE_SPRING_GREEN) / 2) {
        return PB_PUP_COLOR_ID_GREEN;
    }
    if (hue < (PBIO_COLOR_HUE_SPRING_GREEN + PBIO_COLOR_HUE_CYAN) / 2) {
        return PB_PUP_COLOR_ID_SPRING_GREEN;
    }
    if (hue < (PBIO_COLOR_HUE_CYAN + PBIO_COLOR_HUE_BLUE) / 2) {
        return PB_PUP_COLOR_ID_CYAN;
    }
    if (hue < (PBIO_COLOR_HUE_BLUE + PBIO_COLOR_HUE_VIOLET) / 2) {
        return PB_PUP_COLOR_ID_BLUE;
    }
    if (hue < (PBIO_COLOR_HUE_VIOLET + PBIO_COLOR_HUE_MAGENTA) / 2) {
        return PB_PUP_COLOR_ID_VIOLET;
    }
    if (hue < (PBIO_COLOR_HUE_MAGENTA + PBIO_COLOR_HUE_MODULO) / 2) {
        return PB_PUP_COLOR_ID_MAGENTA;
    }
    return PB_PUP_COLOR_ID_RED;
}
