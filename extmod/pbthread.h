// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef PYBRICKS_INCLUDED_PBTHREAD_H
#define PYBRICKS_INCLUDED_PBTHREAD_H

#include "py/mpconfig.h"
#include "py/mpthread.h"

#if MICROPY_PY_THREAD

static inline void pb_thread_enter() {
    extern mp_thread_mutex_t pbthread_mutex;
    mp_thread_mutex_lock(&pbthread_mutex, 1);
}

static inline void pb_thread_exit() {
    extern mp_thread_mutex_t pbthread_mutex;
    mp_thread_mutex_unlock(&pbthread_mutex);
}

static inline void pb_thread_init() {
    extern mp_thread_mutex_t pbthread_mutex;
    mp_thread_mutex_init(&pbthread_mutex);
}

#else // MICROPY_PY_THREAD

static inline void pb_thread_enter() { }
static inline void pb_thread_exit() { }
static inline void pb_thread_init() { }

#endif // MICROPY_PY_THREAD

#endif // PYBRICKS_INCLUDED_PBTHREAD_H
