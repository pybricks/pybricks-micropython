// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdio.h>

#include <btstack.h>

#include <tinytest_macros.h>
#include <tinytest.h>

#include <pbdrv/bluetooth.h>

#include <pbio/os.h>
#include <pbio/util.h>
#include <pbsys/host.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <test-pbio.h>

static pbio_error_t test_bluetooth(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    pbsys_host_init();

    // power should be initialized by the time pbdrv completes.
    tt_want_uint_op(pbio_test_bluetooth_get_control_state(), ==, PBIO_TEST_BLUETOOTH_STATE_ON);

    pbdrv_bluetooth_start_advertising(true);
    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_await_advertise_or_scan_command(&sub, NULL));

    tt_want(pbio_test_bluetooth_is_advertising_enabled());

    pbio_test_bluetooth_connect();

    PBIO_OS_AWAIT_UNTIL(state, pbio_test_bluetooth_is_connected());

    // TODO: enable pybricks service notifications and do concurrent pybricks service and uart service calls

    pbio_test_bluetooth_enable_pybricks_service_notifications();

    static const char *test_data_1 = "test\n";
    static const char *test_data_2 = "test2\n";
    static uint32_t size;

    PBIO_OS_AWAIT_UNTIL(state, ({
        size = strlen(test_data_1);
        pbdrv_bluetooth_tx((const uint8_t *)test_data_1, &size) == PBIO_SUCCESS;
    }));

    tt_want_uint_op(size, ==, strlen(test_data_1));

    // this next data should get pushed in the buffer too
    PBIO_OS_AWAIT_UNTIL(state, ({
        size = strlen(test_data_2);
        pbdrv_bluetooth_tx((const uint8_t *)test_data_2, &size) == PBIO_SUCCESS;
    }));

    tt_want_uint_op(size, ==, strlen(test_data_2));

    static const char *test_data_3 = "\x06test3\n";
    size = strlen(test_data_3);
    pbio_test_bluetooth_send_pybricks_command((const uint8_t *)test_data_3, size);

    static uint8_t rx_data[20];

    PBIO_OS_AWAIT_UNTIL(state, ({
        size = PBIO_ARRAY_SIZE(rx_data);
        pbsys_host_stdin_read(rx_data, &size) == PBIO_SUCCESS;
    }));

    tt_want_uint_op(size, ==, strlen("test3\n"));
    tt_want_int_op(strncmp("test3\n", (const char *)rx_data, size), ==, 0);

    // enabling notifications on Pybricks command characteristic should send
    // a notification right away if status is non-zero
    pbsys_status_set(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING);
    pbio_test_bluetooth_enable_pybricks_service_notifications();

    static uint32_t count;
    count = pbio_test_bluetooth_get_pybricks_service_notification_count();

    PBIO_OS_AWAIT_UNTIL(state, ({
        pbio_test_bluetooth_get_pybricks_service_notification_count() != count;
    }));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

struct testcase_t pbdrv_bluetooth_tests[] = {
    PBIO_PT_THREAD_TEST_WITH_PBIO_OS(test_bluetooth),
    END_OF_TESTCASES
};
