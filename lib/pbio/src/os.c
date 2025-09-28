// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <stdbool.h>
#include <stdio.h>

#include <pbio/os.h>
#include "pbio_os_config.h"

#include <pbio/util.h>

#include <pbdrv/clock.h>

/**
 * Sets the timer to expire after the specified duration.
 *
 * The 1ms interrupt polls all processes, so no special events are needed.
 *
 * @param timer     The timer to initialize.
 * @param duration  The duration in milliseconds.
 */
void pbio_os_timer_set(pbio_os_timer_t *timer, uint32_t duration) {
    timer->start = pbdrv_clock_get_ms();
    timer->duration = duration;
}

/**
 * Extends the timeout by incrementing the start time by the duration.
 *
 * Duration must have been set by ::pbio_os_timer_set previously.
 *
 * @param timer     The timer whose timeout to extend.
 */
void pbio_os_timer_extend(pbio_os_timer_t *timer) {
    timer->start += timer->duration;
}

/**
 * Whether the timer has expired.
 *
 * @param timer     The timer to check.
 * @return          Whether the timer has expired.
 */
bool pbio_os_timer_is_expired(pbio_os_timer_t *timer) {
    return pbio_util_time_has_passed(pbdrv_clock_get_ms(), timer->start + timer->duration);
}

/**
 * Whether a poll request is pending.
 */
static volatile bool poll_request_is_pending = false;

/**
 * Request that the event loop polls all processes.
 */
void pbio_os_request_poll(void) {
    poll_request_is_pending = true;
}


/**
 * Placeholder thread that does nothing and never completes.
 *
 * @param [in] state The protothread state.
 * @param [in] context The context.
 */
pbio_error_t pbio_port_process_none_thread(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_AGAIN;
}

static pbio_os_process_t *process_list = NULL;

static void add_process(pbio_os_process_t *process) {

    pbio_os_process_t **pp = &process_list;

    while (*pp) {
        if (*pp == process) {
            // Already in the list.
            return;
        }
        pp = &(*pp)->next;
    }

    // Insert at end.
    *pp = process;
    process->next = NULL;
}

/**
 * Adds a process to the list of processes to run and starts it soon.
 *
 * @param process   The process to start. Can be an existing process which will be reset.
 * @param func      The process thread function.
 * @param context   The context to pass to the process.
 */
void pbio_os_process_start(pbio_os_process_t *process, pbio_os_process_func_t func, void *context) {

    // Add the new process to the end of the list if not already in it.
    add_process(process);

    process->context = context;
    process->err = PBIO_ERROR_AGAIN;
    process->state = 0;
    process->request = PBIO_OS_PROCESS_REQUEST_TYPE_NONE;
    process->func = func;

    // Request a poll to start the process soon, running to its first yield.
    pbio_os_request_poll();
}

/**
 * Makes a request to a process.
 *
 * It is like sending a signal to the process. It is up to the process to
 * respond or ignore it.
 *
 * @param process   The process.
 * @param request   The request to make.
 */
void pbio_os_process_make_request(pbio_os_process_t *process, pbio_os_process_request_type_t request) {
    process->request = request;
    pbio_os_request_poll();
}

/**
 * Drives the event loop once: Runs one iteration of all processes.
 *
 * Can be used in hooks from blocking loops.
 *
 * @return          Whether there are more pending poll requests.
 */
bool pbio_os_run_processes_once(void) {

    // DELETEME: Legacy hook to drive Contiki processes until all are migrated.
    extern int process_run(void);
    bool pbio_event_pending = process_run();

    if (!poll_request_is_pending) {

        // DELETEME: Legacy pbio event loop hook until all processes migrated.
        if (pbio_event_pending) {
            return true;
        }

        return false;
    }

    poll_request_is_pending = false;

    pbio_os_process_t *process = process_list;
    while (process) {
        // Run one iteration of the process if not yet completed or errored.
        if (process->err == PBIO_ERROR_AGAIN) {
            process->err = process->func(&process->state, process->context);
        }
        process = process->next;
    }

    // Poll requests may have been set while running the processes.
    return poll_request_is_pending || pbio_event_pending;
}

/**
 * Drives the event loop from code that is waiting or sleeping.
 *
 * Expected to be called in a loop. This will keep running the event loop but
 * enter a low power mode when possible. It will sleep for at most one
 * millisecond.
 */
void pbio_os_run_processes_and_wait_for_event(void) {

    // Run the event loop until there is no more pending poll request.
    while (pbio_os_run_processes_once()) {
        ;
    }

    // It is possible that an interrupt occurs *just now* and sets the
    // poll_request_is_pending flag. If not, we can call wait_for_interrupt(),
    // which still wakes up the CPU on interrupt even though interrupts are
    // otherwise disabled.
    pbio_os_irq_flags_t irq_flags = pbio_os_hook_disable_irq();

    // DELETEME: Legacy hook for pbio event loop that plays the same role as
    // the pending flag. Here it ensures we don't enter sleep if there are
    // pending events. Can be removed when all processes are migrated.
    extern int process_nevents(void);
    if (process_nevents()) {
        poll_request_is_pending = true;
    }

    if (!poll_request_is_pending) {
        pbio_os_hook_wait_for_interrupt(irq_flags);
    }
    pbio_os_hook_enable_irq(irq_flags);

    // Since this function is expected to be called in a loop, pending events
    // will be handled right away on the next entry. If not, then they will be
    // handled "soon".
}
