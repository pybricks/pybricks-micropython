
// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk
// Copyright (c) 2020 David Lechner

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
static volatile bool stopping_thread = false;
static pthread_t task_caller_thread;

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

    return NULL;
}

// Pybricks initialization tasks
void pybricks_init() {
    GError *error = NULL;
    if (!grx_set_mode_default_graphics(FALSE, &error)) {
        fprintf(stderr, "Could not initialize graphics. Be sure to run using `brickrun -r -- pybricks-micropython`.\n");
        exit(1);
    }
    grx_clear_screen(GRX_COLOR_WHITE);
    GrxLineOptions options = { .width = 10 };
    gint margin = MIN(grx_get_width(), grx_get_height()) / 10;
    grx_draw_box_with_options(margin, margin, grx_get_max_x() - margin, grx_get_max_y() - margin, &options);

    pbio_init();
    pbio_light_on_with_pattern(PBIO_PORT_SELF, PBIO_LIGHT_COLOR_GREEN, PBIO_LIGHT_PATTERN_BREATHE); // TODO: define PBIO_LIGHT_PATTERN_EV3_RUN (Or, discuss if we want to use breathe for EV3, too)
    pthread_create(&task_caller_thread, NULL, task_caller, NULL);
}

// Pybricks deinitialization tasks
void pybricks_deinit(){
    // Signal motor thread to stop and wait for it to do so.
    stopping_thread = true;
    pthread_join(task_caller_thread, NULL);
    pbio_deinit();
}
