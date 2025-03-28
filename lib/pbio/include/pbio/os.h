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
 *
 * Caution: it is conceptually very similar to Contiki but it cannot be used as
 * a drop-in replacement.
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
 * Protothread state. Effectively the checkpoint (line number) in the current
 * file where it yields so it can jump there to resume later.
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
     * The protothread function.
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
 * Reset a protothread state back to the start.
 *
 * Must be done prior to starting to execute the protothread.
 *
 * @param [in]  state    Protothread state.
 */
#define PBIO_OS_ASYNC_RESET(state) *state = 0;

/**
 * Declare the start of a protothread inside the C function implementing the
 * protothread.
 *
 * This macro is used to declare the starting point of a protothread. It should
 * be placed at the start of the function in which the protothread runs. All C
 * statements above this macro will be executed each time the protothread is
 * scheduled.
 *
 * This macro has the effect of resuming (jumping to) the most recently set
 * checkpoint in the protothread and executing the code from there until the
 * next yield or return statement.
 *
 * The do_yield_now variable is used only to facilitate the AWAIT_ONCE macro.
 * It shouldn't take any space if unused and optimizations are enabled.
 *
 * @param [in]  state    Protothread state.
 */
#define PBIO_OS_ASYNC_BEGIN(state) { char do_yield_now = 0; (void)do_yield_now; switch (*state) { case 0:

/**
 * Sets a protothread checkpoint state to the current line number so we can
 * continue from (jump) here later if we yield.
 *
 * This is used as part of several other yield-like macros such as yielding
 * until a condition is true. This macro is not typically used directly in user
 * code.
 *
 * NB: Do not use within another switch() statement!
 *
 * @param [in]  state    Protothread state.
 */
#define PBIO_OS_ASYNC_SET_CHECKPOINT(state) *state = __LINE__; __attribute__((fallthrough)); case __LINE__:

/**
 * Declare the end of a protothread and returns with given code.
 *
 * This macro is used for declaring that a protothread ends. It must
 * always be used together with a matching PBIO_OS_ASYNC_BEGIN() macro.
 *
 * @param [in]  err    Error code to return.
 */
#define PBIO_OS_ASYNC_END(err) }; return err; }

/**
 * Yields the protothread while the specified condition is true.
 *
 * @param [in]  state       Protothread state.
 * @param [in]  condition   The condition (or statement expression) to check.
 */
#define PBIO_OS_AWAIT_WHILE(state, condition)    \
    do {                                         \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);     \
        if (condition) {                         \
            return PBIO_ERROR_AGAIN;             \
        }                                        \
    } while (0)

/**
 * Yields the protothread until the specified condition is true.
 *
 * @param [in]  state     Protothread state.
 * @param [in]  condition The condition (or statement expression) to check.
 */
#define PBIO_OS_AWAIT_UNTIL(state, condition)    \
    do {                                         \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);     \
        if (!(condition)) {                      \
            return PBIO_ERROR_AGAIN;             \
        }                                        \
    } while (0)

/**
 * Awaits another (sub) protothread within the current (host) protothread until
 * it completes successfully or returns an error.
 *
 * @param [in]  host_state         State of host protothread in which this macro is used.
 * @param [in]  sub_state          State of the sub protothread.
 * @param [in]  calling_statement  The statement to await.
 */
#define PBIO_OS_AWAIT(host_state, sub_state, calling_statement)  \
    do {                                                         \
        PBIO_OS_ASYNC_RESET((sub_state));                        \
        PBIO_OS_ASYNC_SET_CHECKPOINT(host_state);                \
        if ((calling_statement) == PBIO_ERROR_AGAIN) {           \
            return PBIO_ERROR_AGAIN;                             \
        }                                                        \
    } while (0)

/**
 * Awaits two protothreads until one of them completes successfully or returns
 * an error.
 *
 * @param [in]  host_state             State of the host protothread in which this macro is used.
 * @param [in]  sub_state_1            State of the first sub protothread.
 * @param [in]  sub_state_2            State of the second sub protothread.
 * @param [in]  calling_statement_1    (Error-assigning) calling statement of the first sub protothread.
 * @param [in]  calling_statement_2    (Error-assigning) calling statement of the second sub protothread.
 */
#define PBIO_OS_AWAIT_RACE(host_state, sub_state_1, sub_state_2, calling_statement_1, calling_statement_2) \
    do {                                                                                                   \
        PBIO_OS_ASYNC_RESET((sub_state_1));                                                                \
        PBIO_OS_ASYNC_RESET((sub_state_2));                                                                \
        PBIO_OS_ASYNC_SET_CHECKPOINT(host_state);                                                          \
        if ((calling_statement_1) == PBIO_ERROR_AGAIN && (calling_statement_2) == PBIO_ERROR_AGAIN) {      \
            return PBIO_ERROR_AGAIN;                                                                       \
        }                                                                                                  \
    } while (0)

/**
 * Yields the protothread here once and polls to request handling again
 * immediately.
 *
 * Can be useful if several protothreads are started at the same time but not
 * in any particular order. PBIO_OS_AWAIT_ONCE_AND_POLL can be used in a loop
 * until the other protothreads have finished initializing as defined by
 * mutually defined state variables.
 *
 * Should be used sparingly as it can cause busy waiting. Processes will keep
 * running, but there will always be another event pending after this is used.
 *
 * @param [in]  state     Protothread state.
 */
#define PBIO_OS_AWAIT_ONCE_AND_POLL(state)      \
    do {                                        \
        do_yield_now = 1;                       \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);    \
        if (do_yield_now) {                     \
            pbio_os_request_poll();             \
            return PBIO_ERROR_AGAIN;            \
        }                                       \
    } while (0)

#define PBIO_OS_AWAIT_MS(state, timer, duration)      \
    do {                                              \
        pbio_os_timer_set(timer, duration);           \
        PBIO_OS_ASYNC_SET_CHECKPOINT(state);          \
        if (!pbio_os_timer_is_expired(timer)) {       \
            return PBIO_ERROR_AGAIN;                  \
        }                                             \
    } while (0)                                       \

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
