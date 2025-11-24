// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdbool.h>
#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbsys/status.h>
#include <test-pbio.h>

#include "../../drv/clock/clock_test.h"

static pbio_error_t test_status(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // use the last valid flag for edge case
    static const pbio_pybricks_status_flags_t test_flag = NUM_PBIO_PYBRICKS_STATUS - 1;

    // ensure flags are initialized as unset
    tt_want(!pbsys_status_test(test_flag));

    // ensure that setting a flag works as expected
    pbsys_status_set(test_flag);
    tt_want(pbsys_status_test(test_flag));
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that debounce works
    PBIO_OS_AWAIT_MS(state, &timer, 9);
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));
    PBIO_OS_AWAIT_MS(state, &timer, 1);
    tt_want(pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that setting a flag again does not reset debounce timer or broadcast event
    pbsys_status_set(test_flag);
    tt_want(pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that clearing a flag works as expected
    pbsys_status_clear(test_flag);
    tt_want(!pbsys_status_test(test_flag));
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that debounce works
    PBIO_OS_AWAIT_MS(state, &timer, 9);
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));
    PBIO_OS_AWAIT_MS(state, &timer, 1);
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that clearing a flag again does not reset debounce timer or broadcast
    pbsys_status_clear(test_flag);
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(pbsys_status_test_debounce(test_flag, false, 10));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

struct testcase_t pbsys_status_tests[] = {
    PBIO_THREAD_TEST(test_status),
    END_OF_TESTCASES
};
