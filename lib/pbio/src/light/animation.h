// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBIO_LIGHT_ANIMATION_H_
#define _PBIO_LIGHT_ANIMATION_H_

#include <stdint.h>

#include <contiki.h>

typedef struct _pbio_light_animation_t pbio_light_animation_t;

/**
 * Implementation-specific callback to apply the animation.
 *
 * Implmetations should update the animation and return the time to wait
 * before moving to the next cell.
 *
 * @param [in]  animation       The animation instance
 * @return                      The time to wait before calling next() again
 */
typedef clock_time_t (*pbio_light_animation_next_t)(pbio_light_animation_t *animation);

struct _pbio_light_animation_t {
    /** Animation update timer. */
    struct etimer timer;
    /** Animation iterator callback. */
    pbio_light_animation_next_t next;
};

void pbio_light_animation_init(pbio_light_animation_t *animation, pbio_light_animation_next_t next);
void pbio_light_animation_start(pbio_light_animation_t *animation);
void pbio_light_animation_stop(pbio_light_animation_t *animation);

#endif // _PBIO_LIGHT_ANIMATION_H_
