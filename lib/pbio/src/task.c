// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

// Asynchronous task scheduler.

#include <assert.h>
#include <stdbool.h>

#include <contiki.h>
#include <contiki-lib.h>

#include <pbio/error.h>
#include <pbio/task.h>

/**
 * Initializes the @p task data structure.
 * @param [in]  task    The uninitialized data structure.
 * @param [in]  thread  The task protothread.
 * @param [in]  context The caller-defined task context.
 */
void pbio_task_init(pbio_task_t *task, pbio_task_thread_t thread, void *context) {
    task->thread = thread;
    task->context = context;
    PT_INIT(&task->pt);
    task->status = PBIO_ERROR_AGAIN;
    task->cancel = false;
}

/**
 * Runs the task protothread until the next yield.
 * @param [in]  task    The task.
 * @returns             True if the protothread has completed, otherwise false.
 *
 * Do not call this on a completed task.
 */
bool pbio_task_run_once(pbio_task_t *task) {
    assert(task->status == PBIO_ERROR_AGAIN);

    if (PT_SCHEDULE(task->thread(&task->pt, task))) {
        return false;
    }

    assert(task->status != PBIO_ERROR_AGAIN);
    assert((task->status == PBIO_ERROR_CANCELED) == task->cancel);

    return true;
}

/**
 * Sets cancel flag for @p task.
 * @param [in]  task The task.
 */
void pbio_task_cancel(pbio_task_t *task) {
    task->cancel = true;
    // FIXME: need a way to poll the process that has the task queue to
    // expedite the cancellation. e.g. if the task is not the first in the
    // queue, it can just be removed from the queue. For btstack, we probably
    // need to create a synthetic event. For other drivers, we can probably
    // do something with contiki events. Since it is driver-specific, this
    // means we probably need to add a cancel callback to the task struct.
}
