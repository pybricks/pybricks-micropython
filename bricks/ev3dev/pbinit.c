
// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/timerfd.h>

#include <contiki.h>

#include <glib.h>
#include <grx-3.0.h>

#include <pbio/dcmotor.h>
#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/control.h>
#include <pbio/main.h>
#include <pbio/light.h>

#include <pybricks/common.h>

#include "py/mpconfig.h"
#include "py/mpthread.h"

#include "pbinit.h"

// Flag that indicates whether we are busy stopping the thread
static volatile bool stopping_thread = false;
static pthread_t task_caller_thread;

// The background thread that keeps firing the task handler
static void *task_caller(void *arg) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = PBIO_CONFIG_CONTROL_LOOP_TIME_MS * 1000000;

    while (!stopping_thread) {
        MP_THREAD_GIL_ENTER();
        pbio_process_events();
        MP_THREAD_GIL_EXIT();

        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        etimer_request_poll();
    }

    return NULL;
}

// Pybricks initialization tasks
void pybricks_init(void) {
    GError *error = NULL;
    if (!grx_set_mode_default_graphics(FALSE, &error)) {
        fprintf(stderr, "Could not initialize graphics. Be sure to run using `brickrun -r -- pybricks-micropython`.\n");
        exit(1);
    }
    grx_clear_screen(GRX_COLOR_WHITE);

    // Screen center
    gint cx = grx_get_width() / 2;
    gint cy = grx_get_height() / 2;

    // One side of the triangle
    gint base = MIN(grx_get_width(), grx_get_height()) * 7 / 16;

    // Perfect circle around triangle: r = base / (2*cos(30))
    // Horizontal distance from center to circle: s = r  * sin(30)
    gint r = (base * 100) / 173;
    gint s = r / 2;

    // Draw larger circle around triangle
    gint width = 5;
    gint cr = r * 3 / 2;
    grx_draw_filled_circle(cx, cy, cr, GRX_COLOR_BLACK);
    grx_draw_filled_circle(cx, cy, cr - width, GRX_COLOR_WHITE);

    GrxPoint triangle[] = {
        {
            // Upper left vertex
            .x = cx - s,
            .y = cy - base / 2
        },
        {
            // Bottom left vertex
            .x = cx - s,
            .y = cy + base / 2
        },
        {
            // Right vertex
            .x = cx + r,
            .y = cy
        }
    };
    grx_draw_filled_convex_polygon(G_N_ELEMENTS(triangle), triangle, GRX_COLOR_BLACK);

    pbio_init();
    extern void ev3dev_status_light_init(void);
    ev3dev_status_light_init();
    pb_package_pybricks_init(true);
    pthread_create(&task_caller_thread, NULL, task_caller, NULL);
}

// Pybricks deinitialization tasks
void pybricks_deinit(void) {
    // Signal motor thread to stop and wait for it to do so.
    stopping_thread = true;
    pthread_join(task_caller_thread, NULL);
}

void pybricks_unhandled_exception(void) {
    pbio_stop_all(false);
    extern void _pb_ev3dev_speaker_beep_off(void);
    _pb_ev3dev_speaker_beep_off();
}
