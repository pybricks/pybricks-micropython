// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

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
    pbio_task_t task;

    pbio_task_init(&task, no_yield_task_thread, NULL);
    pbio_task_run_once(&task);

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

// This demonstrates how to implement task cancellation.
static void test_task_cancellation(void *env) {
    pbio_task_t task;
    uint32_t call_count = 0;

    pbio_task_init(&task, cancel_task_thread, &call_count);
    pbio_task_run_once(&task);

    tt_want_uint_op(call_count, ==, 1);
    tt_want_uint_op(task.status, ==, PBIO_ERROR_AGAIN);

    pbio_task_run_once(&task);

    tt_want_uint_op(call_count, ==, 2);
    tt_want_uint_op(task.status, ==, PBIO_ERROR_AGAIN);

    pbio_task_cancel(&task);

    tt_want_uint_op(call_count, ==, 3);
    tt_want_uint_op(task.status, ==, PBIO_ERROR_CANCELED);
}

struct testcase_t pbio_task_tests[] = {
    PBIO_TEST(test_no_yield_task),
    PBIO_TEST(test_task_cancellation),
    END_OF_TESTCASES
};
