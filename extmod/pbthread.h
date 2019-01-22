/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
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
