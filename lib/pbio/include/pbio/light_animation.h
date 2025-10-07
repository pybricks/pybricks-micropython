// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#ifndef _PBIO_LIGHT_ANIMATION_H_
#define _PBIO_LIGHT_ANIMATION_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/os.h>
#include <pbio/config.h>

typedef struct _pbio_light_animation_t pbio_light_animation_t;

/**
 * Implementation-specific callback to apply the animation.
 *
 * Implementations should update the animation and return the time to wait
 * before moving to the next cell.
 *
 * @param [in]  animation       The animation instance
 * @return                      The time to wait before calling next() again
 */
typedef uint32_t (*pbio_light_animation_next_t)(pbio_light_animation_t *animation);

struct _pbio_light_animation_t {
    /** Animation update timer. */
    pbio_os_timer_t timer;
    /** Animation iterator callback. */
    pbio_light_animation_next_t next;
    /** Linked list */
    pbio_light_animation_t *next_animation;
};

#if PBIO_CONFIG_LIGHT

void pbio_light_animation_init_module(void);
void pbio_light_animation_init(pbio_light_animation_t *animation, pbio_light_animation_next_t next);
void pbio_light_animation_start(pbio_light_animation_t *animation);
void pbio_light_animation_stop(pbio_light_animation_t *animation);
void pbio_light_animation_stop_all(void);
bool pbio_light_animation_is_started(pbio_light_animation_t *animation);

#else // PBIO_CONFIG_LIGHT

static inline void pbio_light_animation_init_module(void) {
}

static inline void pbio_light_animation_init(pbio_light_animation_t *animation, pbio_light_animation_next_t next) {
}

static inline void pbio_light_animation_start(pbio_light_animation_t *animation) {
}

static inline void pbio_light_animation_stop(pbio_light_animation_t *animation) {
}

static inline void pbio_light_animation_stop_all(void) {
}

static inline bool pbio_light_animation_is_started(pbio_light_animation_t *animation) {
    return false;
}

#endif // PBIO_CONFIG_LIGHT


#endif // _PBIO_LIGHT_ANIMATION_H_
