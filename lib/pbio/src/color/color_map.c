// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pbio/color.h>
#include <pbio/color_map.h>
#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/util.h>

#if PBIO_CONFIG_PORT_DCM_NUM_DEV

// One map for each port that can have a sensor attached. These are static so
// that the selected colors outlive the user program that sets them.
static pbio_color_map_t color_maps[PBIO_CONFIG_PORT_DCM_NUM_DEV];

/**
 * Gets the color map instance for the given port index.
 *
 * @param [in]  index   The port index.
 * @return              The color map, or @c NULL if this port cannot have
 *                      sensors attached.
 */
pbio_color_map_t *pbio_color_map_init_instance(uint8_t index) {
    if (index >= PBIO_ARRAY_SIZE(color_maps)) {
        return NULL;
    }
    pbio_color_map_t *map = &color_maps[index];
    pbio_color_map_set_default(map);
    return map;
}

#else // PBIO_CONFIG_PORT_DCM_NUM_DEV

pbio_color_map_t *pbio_color_map_init_instance(uint8_t index) {
    return NULL;
}

#endif // PBIO_CONFIG_PORT_DCM_NUM_DEV

/**
 * Resets a color map to the colors that are detected by default.
 *
 * @param [in]  map     The color map.
 */
void pbio_color_map_set_default(pbio_color_map_t *map) {
    static const pbio_color_t default_colors[] = {
        PBIO_COLOR_RED,
        PBIO_COLOR_YELLOW,
        PBIO_COLOR_GREEN,
        PBIO_COLOR_BLUE,
        PBIO_COLOR_WHITE,
        PBIO_COLOR_NONE,
    };
    pbio_color_map_set(map, default_colors, PBIO_ARRAY_SIZE(default_colors));
}

/**
 * Sets the colors that should be detected.
 *
 * @param [in]  map         The color map.
 * @param [in]  colors      The colors to detect.
 * @param [in]  num_colors  How many colors, at most ::PBIO_COLOR_MAP_NUM_COLORS.
 * @return                  ::PBIO_SUCCESS on success.
 *                          ::PBIO_ERROR_INVALID_ARG if there are too many colors.
 */
pbio_error_t pbio_color_map_set(pbio_color_map_t *map, const pbio_color_t *colors, size_t num_colors) {

    if (num_colors > PBIO_COLOR_MAP_NUM_COLORS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    map->num_colors = num_colors;
    map->use_bicone = false;

    for (size_t i = 0; i < num_colors; i++) {
        map->colors[i] = colors[i];

        // If the user only provides fully saturated colors (hue, 100, 100)
        // and/or fully desaturated colors (0, 0, value), use a simplified
        // heuristic matcher for better default results that are distance
        // independent. Otherwise use a bicone color distance measure.
        bool idealized_grayscale = pbio_color_get_s(colors[i]) == 0 && pbio_color_get_h(colors[i]) == 0;
        bool idealized_color = pbio_color_get_s(colors[i]) == 100 && pbio_color_get_v(colors[i]) == 100;
        if (!idealized_grayscale && !idealized_color) {
            map->use_bicone = true;
        }
    }

    return PBIO_SUCCESS;
}

/**
 * Gets the color in the map that most closely matches the given color.
 *
 * @param [in]  map     The color map.
 * @param [in]  color   The measured color.
 * @return              The closest color in the map, or ::PBIO_COLOR_NONE if
 *                      the map is empty.
 */
pbio_color_t pbio_color_map_find(const pbio_color_map_t *map, pbio_color_t color) {

    pbio_color_distance_func_t distance_func = map->use_bicone ?
        pbio_color_get_distance_bicone_squared :
        pbio_color_get_distance_saturation_heuristic;

    pbio_color_t match = PBIO_COLOR_NONE;
    int32_t cost_min = INT32_MAX;

    for (uint8_t i = 0; i < map->num_colors; i++) {
        int32_t cost_now = distance_func(color, map->colors[i]);
        if (cost_now < cost_min) {
            cost_min = cost_now;
            match = map->colors[i];
        }
    }

    return match;
}
