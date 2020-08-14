// SPDX-License-Identifier: MIT
// Copyright 2020 The Pybricks Authors

#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbsys/status.h>

void test_status(void *env) {
    // use the last valid flag for edge case
    const pbsys_status_t test_flag = NUM_PBSYS_STATUS - 1;

    // ensure flags are initalized as unset
    tt_want(!pbsys_status_test(test_flag));

    // ensure that setting a flag works as expected
    pbsys_status_set(test_flag);
    tt_want(pbsys_status_test(test_flag));
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));
    clock_wait(clock_from_msec(10));
    tt_want(pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that setting a flag again does not reset debounce timer
    pbsys_status_set(test_flag);
    tt_want(pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that clearing a flag works as expected
    pbsys_status_clear(test_flag);
    tt_want(!pbsys_status_test(test_flag));
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(!pbsys_status_test_debounce(test_flag, false, 10));
    clock_wait(clock_from_msec(10));
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(pbsys_status_test_debounce(test_flag, false, 10));

    // ensure that clearing a flag again does not reset debounce timer
    pbsys_status_clear(test_flag);
    tt_want(!pbsys_status_test_debounce(test_flag, true, 10));
    tt_want(pbsys_status_test_debounce(test_flag, false, 10));
}
