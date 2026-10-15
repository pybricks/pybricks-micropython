// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

/**
 * @addtogroup ColorMap pbio/color_map: Color matching
 *
 * Matches measured colors to the set of colors that should be detected.
 * @{
 */

#ifndef _PBIO_COLOR_MAP_H_
#define _PBIO_COLOR_MAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pbio/color.h>
#include <pbio/error.h>

/** Maximum number of colors that a color map can hold. */
#define PBIO_COLOR_MAP_NUM_COLORS (8)

/** The set of colors that a sensor should detect. */
typedef struct {
    /** The colors to choose from when matching. */
    pbio_color_t colors[PBIO_COLOR_MAP_NUM_COLORS];
    /** How many entries of #colors are in use. */
    uint8_t num_colors;
    /**
     * Whether to use the bicone distance instead of the saturation heuristic.
     * Depends only on the colors, so it is decided when they are set.
     */
    bool use_bicone;
} pbio_color_map_t;

pbio_color_map_t *pbio_color_map_init_instance(uint8_t index);

void pbio_color_map_set_default(pbio_color_map_t *map);

pbio_error_t pbio_color_map_set(pbio_color_map_t *map, const pbio_color_t *colors, size_t num_colors);

pbio_color_t pbio_color_map_find(const pbio_color_map_t *map, pbio_color_t color);

#endif // _PBIO_COLOR_MAP_H_

/** @} */
