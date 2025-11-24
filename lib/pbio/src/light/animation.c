// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_LIGHT

#include <assert.h>
#include <stdbool.h>

#include <pbio/light_animation.h>
#include <pbio/os.h>
#include <pbio/util.h>

/**
 * This is used as a value for the next_animation field to indicate when an
 * animation is stopped.
 */
#define PBIO_LIGHT_ANIMATION_STOPPED ((pbio_light_animation_t *)1)

static pbio_light_animation_t *pbio_light_animation_list_head;

/**
 * Initializes required fields of an animation data structure.
 * @param [in]  animation       The animation instance
 * @param [in]  next            The animation update callback
 */
void pbio_light_animation_init(pbio_light_animation_t *animation, pbio_light_animation_next_t next) {
    animation->next = next;
    animation->next_animation = PBIO_LIGHT_ANIMATION_STOPPED;
}

static pbio_error_t pbio_light_animation_poll_handler(pbio_os_state_t *state, void *context) {
    // For every active animation, go to the next frame if timer expired.
    for (pbio_light_animation_t *a = pbio_light_animation_list_head; a != NULL; a = a->next_animation) {
        if (pbio_os_timer_is_expired(&a->timer)) {
            pbio_os_timer_set(&a->timer, a->next(a));
        }
    }
    return PBIO_ERROR_AGAIN;
}

/**
 * Starts an animation in the background.
 *
 * The animation instance must have been initialized with pbio_light_animation_init().
 *
 * The animation must be stopped with pbio_light_animation_stop() before calling
 * pbio_light_animation_start() again.
 *
 * @param [in] animation    The animation instance.
 */
void pbio_light_animation_start(pbio_light_animation_t *animation) {
    assert(animation->next_animation == PBIO_LIGHT_ANIMATION_STOPPED);

    // Insert at head of active list.
    animation->next_animation = pbio_light_animation_list_head;
    pbio_light_animation_list_head = animation;

    // Start process if it wasn't running already.
    static pbio_os_process_t animation_process;
    if (animation_process.err != PBIO_ERROR_AGAIN) {
        pbio_os_process_start(&animation_process, pbio_light_animation_poll_handler, NULL);
    }

    // Fake a timer event to load the first cell.
    pbio_os_timer_set(&animation->timer, 0);
    pbio_os_request_poll();

    assert(animation->next_animation != PBIO_LIGHT_ANIMATION_STOPPED);
}

/**
 * Stops an animation.
 *
 * This must be called once for each call to pbio_light_animation_start().
 *
 * @param [in] animation    The animation instance.
 */
void pbio_light_animation_stop(pbio_light_animation_t *animation) {
    assert(pbio_light_animation_list_head != NULL);
    assert(animation->next_animation != PBIO_LIGHT_ANIMATION_STOPPED);

    if (pbio_light_animation_list_head == animation) {
        pbio_light_animation_list_head = animation->next_animation;
    } else {
        for (pbio_light_animation_t *a = pbio_light_animation_list_head; a != NULL; a = a->next_animation) {
            if (a->next_animation == animation) {
                a->next_animation = animation->next_animation;
                break;
            }
        }
    }

    animation->next_animation = PBIO_LIGHT_ANIMATION_STOPPED;
}

/**
 * Stops all animations.
 *
 * This is intended to stop all animations when a user program is interrupted
 * and pbio_light_animation_stop() will no longer be called.
 */
void pbio_light_animation_stop_all(void) {
    while (pbio_light_animation_list_head) {
        pbio_light_animation_stop(pbio_light_animation_list_head);
    }
}

/**
 * Tests if the animation has been started.
 *
 * @param [in] animation    The animation instance.
 * @return                  *true* if the animation is started, otherwise *false*.
 */
bool pbio_light_animation_is_started(pbio_light_animation_t *animation) {
    return animation->next_animation != PBIO_LIGHT_ANIMATION_STOPPED;
}


#endif // PBIO_CONFIG_LIGHT
