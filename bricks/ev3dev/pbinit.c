
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk
// Copyright (c) 2020 David Lechner

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/timerfd.h>

#include <glib.h>
#include <grx-3.0.h>

#include <pbio/main.h>
#include <pbio/light.h>

#include "py/mpconfig.h"
#include "py/mpthread.h"

#include "pbinit.h"

#define PERIOD_MS 10

// Flag that indicates whether we are busy stopping the thread
volatile bool stopping_thread = false;

// The background thread that keeps firing the task handler
static void *task_caller(void *arg) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = PERIOD_MS * 1000000;

    while (!stopping_thread) {
        MP_THREAD_GIL_ENTER();
        while (pbio_do_one_event()) { }
        MP_THREAD_GIL_EXIT();

        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
    }

    // Signal that shutdown is complete
    stopping_thread = false;
    return NULL;
}

static guint startup_animation_source;

static gboolean update_startup_animation(gpointer user_data) {
    static GrxContext *context = NULL;
    static gint angle = 0;

    if (!context) {
        context = grx_context_new(grx_get_screen_width(), grx_get_screen_height(), NULL, NULL);
        if (!context) {
            g_warning("Failed to create graphics context for animation");
            startup_animation_source = 0;
            return G_SOURCE_REMOVE;
        }
    }

    grx_set_current_context(context);

    gint x = grx_get_width() / 2;
    gint y = grx_get_height() / 2;
    gint r = 9 * MIN(x, y) / 10;
    gint end = angle - 45;

    grx_clear_context(GRX_COLOR_WHITE);
    grx_draw_filled_circle_arc(x, y, r, angle * 10, end * 10,
        GRX_ARC_STYLE_CLOSED_RADIUS, GRX_COLOR_BLACK);
    grx_draw_filled_circle(x, y, 2 * r / 3, GRX_COLOR_WHITE);
    grx_context_bit_blt(grx_get_screen_context(), 0, 0, NULL, 0, 0,
        grx_get_max_x(), grx_get_max_y(), GRX_COLOR_MODE_WRITE);

    angle += 5;

    return G_SOURCE_CONTINUE;
}

void pbricks_end_startup_animation() {
    if (startup_animation_source) {
        g_source_remove(startup_animation_source);
        startup_animation_source = 0;
    }
}

// Pybricks initialization tasks
void pybricks_init() {
    GError *error = NULL;
    if (!grx_set_mode_default_graphics(FALSE, &error)) {
        fprintf(stderr, "Could not initialize graphics. Be sure to run using `brickrun -r -- pybricks-micropython`.\n");
        exit(1);
    }
    grx_clear_screen(GRX_COLOR_WHITE);
    update_startup_animation(NULL);
    startup_animation_source = g_timeout_add(200, update_startup_animation, NULL);

    pbio_init();
    pbio_light_on_with_pattern(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_GREEN, PBIO_LIGHT_PATTERN_BREATHE); // TODO: define PBIO_LIGHT_PATTERN_EV3_RUN (Or, discuss if we want to use breathe for EV3, too)
    pthread_t task_caller_thread;
    pthread_create(&task_caller_thread, NULL, task_caller, NULL);
}

// Pybricks deinitialization tasks
void pybricks_deinit(){
    // Signal motor thread to stop and wait for it to do so.
    stopping_thread = true;
    while (stopping_thread);
    pbio_deinit();
}
