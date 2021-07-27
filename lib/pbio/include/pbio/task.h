// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup Task Tasks
 *
 * Framework for scheduling asynchronous tasks.
 *
 * @{
 */
#ifndef _PBIO_TASK_H_
#define _PBIO_TASK_H_

#include <stdbool.h>

#include <contiki.h>
#include <contiki-lib.h>

#include <pbio/error.h>

/** Task data structure. */
typedef struct _pbio_task_t pbio_task_t;

/** Task protothread function. */
typedef PT_THREAD((*pbio_task_thread_t)(struct pt *pt, pbio_task_t *task));

/** Task data structure fields. */
struct _pbio_task_t {
    /** Linked list node (internal use). */
    pbio_task_t *next;
    /** Task protothread. */
    pbio_task_thread_t thread;
    /** Caller-defined context data structure. */
    void *context;
    /** Protothread state (internal use). */
    struct pt pt;
    /**
     * Task status. ::PBIO_ERROR_AGAIN indicates not done, ::PBIO_SUCCESS indicates
     * that the task completed successfully, ::PBIO_ERROR_CANCELED indicates that
     * task was canceled, other errors indicate that the task failed.
     */
    pbio_error_t status;
    /** Flag for requesting cancelation. */
    bool cancel;
};

void pbio_task_init(pbio_task_t *task, pbio_task_thread_t thread, void *context);
void pbio_task_cancel(pbio_task_t *task);
void pbio_task_start(list_t queue, pbio_task_t *task);
void pbio_task_queue_run_once(list_t queue);

#endif // _PBIO_TASK_H_

/** @} */
