/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
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

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stream.h"

void mp_hal_stdout_tx_flush(void) {
    // currently not buffered
}

void pb_ev3dev_hal_delay_ms(mp_uint_t ms) {
    struct timespec ts = {
        .tv_sec = ms / 1000,
        .tv_nsec = ms % 1000 * 1000000,
    };
    struct timespec remain;
    for (;;) {
        mp_handle_pending(true);
        MP_THREAD_GIL_EXIT();
        int ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &remain);
        MP_THREAD_GIL_ENTER();
        if (ret == EINTR) {
            ts = remain;
            continue;
        }
        assert(ret == 0);
        break;
    }
}

uintptr_t mp_hal_stdio_poll(uintptr_t flags) {
    struct pollfd fds[] = {
        { .fd = STDIN_FILENO, .events = flags & MP_STREAM_POLL_RD ? POLLIN : 0, },
        { .fd = STDOUT_FILENO, .events = flags & MP_STREAM_POLL_WR ? POLLOUT : 0, },
    };
    int ret;

    MP_HAL_RETRY_SYSCALL(ret, poll(fds, MP_ARRAY_SIZE(fds), 0), mp_raise_OSError(err));

    uintptr_t rflags = 0;

    if (ret > 0) {
        if (fds[0].revents & POLLIN) {
            rflags |= MP_STREAM_POLL_RD;
        }
        if (fds[1].revents & POLLOUT) {
            rflags |= MP_STREAM_POLL_WR;
        }
    }

    return rflags;
}
