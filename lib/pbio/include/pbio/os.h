// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2004-2005, Swedish Institute of Computer Science
// Copyright (c) 2025 The Pybricks Authors

/*
 * Copyright (c) 2004-2005, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUASYNCION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * Implementation of local continuations based on switch() statement
 *
 * This implementation of local continuations uses the C switch()
 * statement to resume execution of a function somewhere inside the
 * function's body. The implementation is based on the fact that
 * switch() statements are able to jump directly into the bodies of
 * control structures such as if() or while() statements.
 *
 * This implementation borrows heavily from Simon Tatham's coroutines
 * implementation in C:
 * http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
 *
 * ---------------------------------------------------------------------------
 *
 * The code in this file stems from Contiki's implementation of protothreads as
 * listed above. This adaptation changes the API to work better with Pybricks.
 */

#ifndef _PBIO_OS_H_
#define _PBIO_OS_H_

#include <stdbool.h>
#include <stdint.h>
#include <pbio/error.h>

/**
 * Millisecond timer.
 */
typedef struct pbio_os_timer_t {
    uint32_t start;
    uint32_t duration;
} pbio_os_timer_t;

void pbio_os_timer_set(pbio_os_timer_t *timer, uint32_t duration);

bool pbio_os_timer_is_expired(pbio_os_timer_t *timer);


/**
 * WARNING! LC implementation using switch() does not work if an
 * PB_LC_SET() is done within another switch() statement!
 */

#define PB_LC_FALLTHROUGH __attribute__((fallthrough));
#define PB_LC_INIT(state) *state = 0;
#define PB_LC_RESUME(state) switch (*state) { case 0:
#define PB_LC_SET(state) *state = __LINE__; PB_LC_FALLTHROUGH; case __LINE__:
#define PB_LC_END() }

/**
 * Protothread state. Effectively the line number in the current file where it
 * yields so it can jump there to resume later.
 */
typedef uint32_t pbio_os_state_t;

typedef pbio_error_t (*pbio_os_process_func_t)(pbio_os_state_t *state, void *context);

typedef struct _pbio_os_process_t pbio_os_process_t;

/**
 * A process.
 */
struct _pbio_os_process_t {
    /**
     * Pointer to the next process in the list.
     */
    pbio_os_process_t *next;
    /**
     * State of the protothread.
     */
    pbio_os_state_t state;
    /**
     * The protothread.
     */
    pbio_os_process_func_t func;
    /**
     * Context passed on each call to the protothread.
     */
    void *context;
    /**
     * Most recent result of running one iteration of the protothread.
     */
    pbio_error_t err;
};

/**
 * Initialize a protothread.
 *
 * Initializes a protothread. Initialization must be done prior to
 * starting to execute the protothread.
 *
 * @param [in]  state    Protothread state.
 */
#define ASYNC_INIT(state)   PB_LC_INIT(state)

/**
 * Declare the start of a protothread inside the C function
 * implementing the protothread.
 *
 * This macro is used to declare the starting point of a
 * protothread. It should be placed at the start of the function in
 * which the protothread runs. All C statements above the ASYNC_BEGIN()
 * invocation will be executed each time the protothread is scheduled.
 *
 * The do_yield_now variable is used only to facilitate the AWAIT_ONCE macro.
 * It shouldn't take any space if unused and optimizations are enabled.
 *
 * @param [in]  state    Protothread state.
 */
#define ASYNC_BEGIN(state) { char do_yield_now = 0; (void)do_yield_now; PB_LC_RESUME(state)

/**
 * Declare the end of a protothread and returns with given code.
 *
 * This macro is used for declaring that a protothread ends. It must
 * always be used together with a matching ASYNC_BEGIN() macro.
 *
 * NB: In contrast to Contiki, this does not call ASYNC_INIT() before exiting.
 *
 * @param [in]  err    Error code to return.
 */
#define ASYNC_END(err) PB_LC_END(); return err; }

/**
 * Yields the protothread while the specified condition is true.
 *
 * @param [in]  state     Protothread state.
 * @param [in]  condition The condition.
 */
#define AWAIT_WHILE(state, condition)            \
    do {                                         \
        PB_LC_SET(state);                        \
        if (condition) {                         \
            return PBIO_ERROR_AGAIN;             \
        }                                        \
    } while (0)

/**
 * Yields the protothread until the specified condition is true.
 *
 * @param [in]  state     Protothread state.
 * @param [in]  condition The condition.
 */
#define AWAIT_UNTIL(state, condition)            \
    do {                                         \
        PB_LC_SET(state);                        \
        if (!(condition)) {                      \
            return PBIO_ERROR_AGAIN;             \
        }                                        \
    } while (0)

/**
 * Awaits a protothread until it is done.
 *
 * @param [in]  state      Protothread state.
 * @param [in]  child      Protothread state of the child.
 * @param [in]  statement  The statement to await.
 */
#define AWAIT(state, child, statement)          \
    do {                                        \
        ASYNC_INIT((child));                    \
        PB_LC_SET(state);                       \
        if ((statement) == PBIO_ERROR_AGAIN) {  \
            return PBIO_ERROR_AGAIN;            \
        }                                       \
    } while (0)

/**
 * Awaits two protothreads until one of them is done.
 *
 * @param [in]  state       Protothread state.
 * @param [in]  child1      Protothread state of the first child.
 * @param [in]  child2      Protothread state of the second child.
 * @param [in]  statement1  The first statement to await.
 * @param [in]  statement2  The second statement to await.
 */
#define AWAIT_RACE(state, child1, child2, statement1, statement2)                    \
    do {                                                                             \
        ASYNC_INIT((child1));                                                        \
        ASYNC_INIT((child2));                                                        \
        PB_LC_SET(state);                                                            \
        if ((statement1) == PBIO_ERROR_AGAIN && (statement2) == PBIO_ERROR_AGAIN) {  \
            return PBIO_ERROR_AGAIN;                                                 \
        }                                                                            \
    } while (0)

/**
 * Yields the protothread once and polls to request handling again immediately.
 *
 * Should be used sparingly as it can cause busy waiting. Processes will keep
 * running, but there is always another event pending.
 *
 * @param [in]  state     Protothread state.
 */
#define AWAIT_ONCE(state)                       \
    do {                                        \
        do_yield_now = 1;                       \
        PB_LC_SET(state);                       \
        if (do_yield_now) {                     \
            pbio_os_request_poll();             \
            return PBIO_ERROR_AGAIN;            \
        }                                       \
    } while (0)

#define AWAIT_MS(state, timer, duration)        \
    do {                                        \
        pbio_os_timer_set(timer, duration);     \
        PB_LC_SET(state);                       \
        if (!pbio_os_timer_is_expired(timer)) { \
            return PBIO_ERROR_AGAIN;            \
        }                                       \
    } while (0)                                 \

bool pbio_os_run_processes_once(void);

void pbio_os_run_while_idle(void);

void pbio_os_request_poll(void);

pbio_error_t pbio_port_process_none_thread(pbio_os_state_t *state, void *context);

void pbio_os_process_start(pbio_os_process_t *process, pbio_os_process_func_t func, void *context);

void pbio_os_process_reset(pbio_os_process_t *process, pbio_os_process_func_t func);

/**
 * Disables interrupts and returns the previous interrupt state.
 *
 * Must be implemented by the platform.
 *
 * @return  The previous interrupt state.
 */
uint32_t pbio_os_hook_disable_irq(void);

/**
 * Enables interrupts.
 *
 * Must be implemented by the platform.
 *
 * @param [in]  flags  The previous interrupt state.
 */
void pbio_os_hook_enable_irq(uint32_t flags);

/**
 * Waits for an interrupt.
 *
 * Must be implemented by the platform.
 */
void pbio_os_hook_wait_for_interrupt(void);

#endif // _PBIO_OS_H_
