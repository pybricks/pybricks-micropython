// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <contiki-lib.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/error.h>
#include <pbio/task.h>
#include <test-pbio.h>

static PT_THREAD(no_yield_task_thread(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

// Tests that a task that does not yield does not get queued.
static void test_no_yield_task(void *env) {
    LIST(queue);

    pbio_task_t task;

    pbio_task_init(&task, no_yield_task_thread, NULL);
    pbio_task_queue_add(queue, &task);

    tt_want_uint_op(list_length(queue), ==, 0);
    tt_want_uint_op(task.status, ==, PBIO_SUCCESS);
}

static PT_THREAD(one_yield_task_thread(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    PT_YIELD(pt);

    task->status = PBIO_SUCCESS;

    PT_END(pt);
}

// Tests that a task is removed from the queue when it is completed.
static void test_task_removed_when_complete(void *env) {
    LIST(queue);

    pbio_task_t task;

    pbio_task_init(&task, one_yield_task_thread, NULL);
    pbio_task_queue_add(queue, &task);

    tt_want_uint_op(list_length(queue), ==, 1);
    tt_want_uint_op(task.status, ==, PBIO_ERROR_AGAIN);

    pbio_task_queue_run_once(queue);

    tt_want_uint_op(list_length(queue), ==, 0);
    tt_want_uint_op(task.status, ==, PBIO_SUCCESS);
}

static PT_THREAD(cancel_task_thread(struct pt *pt, pbio_task_t *task)) {
    uint32_t *call_count = task->context;

    (*call_count)++;

    PT_BEGIN(pt);

    PT_WAIT_UNTIL(pt, task->cancel);
    task->status = PBIO_ERROR_CANCELED;

    PT_END(pt);
}

// This demonstrates how to implement task cancelation.
static void test_task_cancelation(void *env) {
    LIST(queue);
    pbio_task_t task;
    uint32_t call_count = 0;

    pbio_task_init(&task, cancel_task_thread, &call_count);
    pbio_task_queue_add(queue, &task);

    tt_want_uint_op(call_count, ==, 1);
    tt_want_uint_op(list_length(queue), ==, 1);
    tt_want_uint_op(task.status, ==, PBIO_ERROR_AGAIN);

    pbio_task_queue_run_once(queue);

    // stays in queue when not canceled yet
    tt_want_uint_op(call_count, ==, 2);
    tt_want_uint_op(list_length(queue), ==, 1);
    tt_want_uint_op(task.status, ==, PBIO_ERROR_AGAIN);

    pbio_task_cancel(&task);
    pbio_task_queue_run_once(queue);

    // since there is no yield after cancelation, call count is only 3 even
    // though both functions above could potentially iterate the task
    tt_want_uint_op(call_count, ==, 3);
    tt_want_uint_op(list_length(queue), ==, 0);
    tt_want_uint_op(task.status, ==, PBIO_ERROR_CANCELED);
}

static PT_THREAD(counting_task_thread(struct pt *pt, pbio_task_t *task)) {
    PT_BEGIN(pt);

    for (;;) {
        // using pointer to hold an int value
        intptr_t *count = (intptr_t *)&task->context;
        (*count)++;

        PT_YIELD(pt);
    }

    PT_END(pt);
}

// This tests removing a task from a queue with more than one task (which can
// be tricky to implement correctly).
static void test_task_removal(void *env) {
    LIST(queue);

    pbio_task_t task1, task2, task3;

    pbio_task_init(&task1, counting_task_thread, (void *)0);
    pbio_task_queue_add(queue, &task1);

    pbio_task_init(&task2, one_yield_task_thread, NULL);
    pbio_task_queue_add(queue, &task2);

    pbio_task_init(&task3, counting_task_thread, (void *)0);
    pbio_task_queue_add(queue, &task3);

    // at this point, all 3 tasks should be queued
    tt_want_uint_op(list_length(queue), ==, 3);

    pbio_task_queue_run_once(queue);

    // only task2 should be removed
    tt_want_uint_op(list_length(queue), ==, 2);
    // other tasks should have been called twice each
    tt_want_uint_op((intptr_t)task1.context, ==, 2);
    tt_want_uint_op((intptr_t)task3.context, ==, 2);
}

struct testcase_t pbio_task_tests[] = {
    PBIO_TEST(test_no_yield_task),
    PBIO_TEST(test_task_removed_when_complete),
    PBIO_TEST(test_task_cancelation),
    PBIO_TEST(test_task_removal),
    END_OF_TESTCASES
};
