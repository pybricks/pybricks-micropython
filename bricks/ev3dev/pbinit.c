
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Laurens Valk
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/timerfd.h>

#include <pbio/main.h>
#include <pbio/light.h>

#include "py/mpthread.h"

#include "pbinit.h"
#include "pbthread.h"

#define PERIOD_MS 10

struct periodic_info {
    int timer_fd;
    unsigned long long wakeups_missed;
};

// Configure timer at specified interval
static int configure_timer_thread(unsigned int period_ms, struct periodic_info *info)
{
    struct itimerspec itval;
    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    info->wakeups_missed = 0;
    info->timer_fd = fd;
    if (fd == -1){
        return fd;
    }
    itval.it_interval.tv_sec = 0;
    itval.it_interval.tv_nsec = period_ms * 1000000;
    itval.it_value.tv_sec = 0;
    itval.it_value.tv_nsec = period_ms * 1000000;
    return timerfd_settime(fd, 0, &itval, NULL);
}

// Wait for timer to complete period
static void wait_period(struct periodic_info *info)
{
    unsigned long long missed;
    int ret;
    // Wait for the next timer event. If we have missed any the number is written to "missed".
    ret = read(info->timer_fd, &missed, sizeof(missed));
    if (ret == -1) {
        perror("Unable to read timer");
        return;
    }
    info->wakeups_missed += missed;
}

// Flag that indicates whether we are busy stopping the thread
volatile bool stopping_thread = false;

// The background thread that keeps firing the task handler
static void *task_caller(void *arg)
{
    struct periodic_info info;
    configure_timer_thread(PERIOD_MS, &info);
    while (!stopping_thread) {
        pb_thread_enter();
        while (pbio_do_one_event()) { }
        pb_thread_exit();
        wait_period(&info); // TODO: check if we should do any waiting here
    }
    // Signal that shutdown is complete
    stopping_thread = false;
    return NULL;
}

// Pybricks initialization tasks
void pybricks_init(){
    pb_thread_init();
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
