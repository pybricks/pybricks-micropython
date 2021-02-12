// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdio.h>

#include <btstack.h>
#include <contiki.h>
#include <tinytest_macros.h>
#include <tinytest.h>

#include <pbsys/bluetooth.h>
#include <test-pbio.h>

static PT_THREAD(test_bluetooth(struct pt *pt)) {
    PT_BEGIN(pt);

    pbsys_bluetooth_init();

    // power should be initalized to off
    tt_want_uint_op(pbio_test_bluetooth_get_control_state(), ==, PBIO_TEST_BLUETOOTH_STATE_OFF);

    // wait for the power on delay
    PT_WAIT_UNTIL(pt, {
        clock_tick(1);
        pbio_test_bluetooth_get_control_state() == PBIO_TEST_BLUETOOTH_STATE_ON;
    });

    PT_WAIT_UNTIL(pt, {
        clock_tick(1);
        pbio_test_bluetooth_is_advertising_enabled();
    });

    pbio_test_bluetooth_connect();

    PT_YIELD(pt);

    // TODO: enable notifications and do concurrent pybricks service and uart service calls

    PT_END(pt);
}

struct testcase_t pbsys_bluetooth_tests[] = {
    PBIO_PT_THREAD_TEST(test_bluetooth),
    END_OF_TESTCASES
};
