// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

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
 */
static bool pbio_task_run_once(pbio_task_t *task) {
    if (PT_SCHEDULE(task->thread(&task->pt, task))) {
        return false;
    }

    assert(task->status != PBIO_ERROR_AGAIN);
    assert((task->status == PBIO_ERROR_CANCELED) == task->cancel);

    return true;
}

/**
 * Adds @p task to @p queue and starts the task.
 * @param [in]  queue   The task queue.
 * @param [in]  task    The task.
 */
void pbio_task_start(list_t queue, pbio_task_t *task) {
    if (!pbio_task_run_once(task)) {
        // only queue the task if it has not completed
        list_add(queue, task);
    }
}

/**
 * Runs each task in the queue until the next yield.
 *
 * If a task is completed, it is removed from the queue.
 */
void pbio_task_queue_run_once(list_t queue) {
    pbio_task_t *next;
    for (pbio_task_t *task = list_head(queue); task != NULL; task = next) {
        // have to save next now in case task is removed from queue
        next = list_item_next(task);

        if (pbio_task_run_once(task)) {
            // remove the task from the queue only if the task is complete
            list_remove(queue, task);
        }
    }
}
