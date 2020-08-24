// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <stdint.h>

#include <contiki.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/light.h>

#include "animation.h"

#ifndef _PBIO_LIGHT_COLOR_LIGHT_H_
#define _PBIO_LIGHT_COLOR_LIGHT_H_

/** Implementation-specific callbacks for a color light. */
typedef struct {
    /** Sets the light to the specified color and brightness. */
    pbio_error_t (*set_hsv)(pbio_color_light_t *light, const pbio_color_hsv_t *hsv);
} pbio_color_light_funcs_t;

/** Data structure for defining a color light instance. */
struct _pbio_color_light_t {
    pbio_light_animation_t animation;
    const pbio_color_light_funcs_t *funcs;
    const pbio_color_light_animation_cell_t *cells;
    uint8_t current_cell;
};

void pbio_color_light_init(pbio_color_light_t *light, const pbio_color_light_funcs_t *funcs);

#endif // _PBIO_LIGHT_COLOR_LIGHT_H_
