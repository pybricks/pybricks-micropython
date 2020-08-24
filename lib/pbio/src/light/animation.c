// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_LIGHT

#include <assert.h>

#include <contiki.h>

#include <pbio/util.h>

#include "animation.h"

PROCESS(pbio_light_animation_process, "light animation");

static uint8_t pbio_light_animation_process_ref_count;

/**
 * Initializes required fields of an animation data structure.
 * @param [in]  animation       The animation instance
 * @param [in]  next            The animation update callback
 */
void pbio_light_animation_init(pbio_light_animation_t *animation, pbio_light_animation_next_t next) {
    animation->next = next;
}

/**
 * Starts an animation in the background.
 *
 * The animation instance must have been initalized with pbio_light_animation_init().
 *
 * The animation must be stopped with pbio_light_animation_stop() before calling
 * pbio_light_animation_start() again.
 *
 * @param [in] animation    The animation instance.
 */
void pbio_light_animation_start(pbio_light_animation_t *animation) {
    if (pbio_light_animation_process_ref_count++ == 0) {
        process_start(&pbio_light_animation_process, NULL);
    }
    // HACK: init timer since we don't call etimer_set()
    timer_restart(&animation->timer.timer);
    // fake a timer event to load the first cell and start the timer
    process_post_synch(&pbio_light_animation_process, PROCESS_EVENT_TIMER, &animation->timer);
}

/**
 * Stops an animation.
 *
 * This must be called once (and only once) for each call to pbio_light_animation_start().
 *
 * @param [in] animation    The animation instance.
 */
void pbio_light_animation_stop(pbio_light_animation_t *animation) {
    assert(pbio_light_animation_process_ref_count > 0);
    etimer_stop(&animation->timer);
    if (--pbio_light_animation_process_ref_count == 0) {
        process_exit(&pbio_light_animation_process);
    }
}

PROCESS_THREAD(pbio_light_animation_process, ev, data) {
    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
        struct etimer *timer = data;
        pbio_light_animation_t *animation = PBIO_CONTAINER_OF(timer, pbio_light_animation_t, timer);
        etimer_reset_with_new_interval(&animation->timer, animation->next(animation));
    }

    PROCESS_END();
}

#endif // PBIO_CONFIG_LIGHT
