// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

    // power should be on by the time we get to the application, where this test runs.
    tt_want_uint_op(pbio_test_bluetooth_get_control_state(), ==, PBIO_TEST_BLUETOOTH_STATE_ON);

    // Should be possible to schedule this command.
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

static uint8_t nus_rx[20];
static uint32_t nus_rx_size;

static void test_nus_handler(const uint8_t *data, uint32_t size) {
    if (size > sizeof(nus_rx)) {
        size = sizeof(nus_rx);
    }
    memcpy(nus_rx, data, size);
    nus_rx_size = size;
}

static pbio_error_t test_bluetooth_nus(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    pbdrv_bluetooth_start_advertising(true);
    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_await_advertise_or_scan_command(&sub, NULL));

    pbio_test_bluetooth_connect();
    PBIO_OS_AWAIT_UNTIL(state, pbio_test_bluetooth_is_connected());

    pbdrv_bluetooth_set_nus_receive_handler(test_nus_handler);

    pbio_test_bluetooth_enable_uart_service_notifications();
    PBIO_OS_AWAIT_UNTIL(state, pbdrv_bluetooth_nus_is_connected());

    static const uint8_t rx[] = { 1, 2, 3, 4 };
    pbio_test_bluetooth_send_uart_data(rx, sizeof(rx));
    PBIO_OS_AWAIT_UNTIL(state, nus_rx_size == sizeof(rx));
    tt_want_int_op(memcmp(nus_rx, rx, sizeof(rx)), ==, 0);

    static uint32_t count;
    count = pbio_test_bluetooth_get_uart_service_notification_count();

    static const uint8_t tx[] = { 9, 8, 7 };
    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_send_nus_notification(&sub, tx, sizeof(tx)));

    PBIO_OS_AWAIT_UNTIL(state, pbio_test_bluetooth_get_uart_service_notification_count() != count);
    tt_want_int_op(pbio_test_bluetooth_get_uart_service_notification_count(), ==, count + 1);

    static const uint8_t tx_long[25] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24,
    };
    count = pbio_test_bluetooth_get_uart_service_notification_count();
    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_send_nus_notification(&sub, tx_long, sizeof(tx_long)));
    tt_want_int_op(pbio_test_bluetooth_get_uart_service_notification_count(), ==, count + 2);

    pbdrv_bluetooth_set_nus_receive_handler(NULL);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

struct testcase_t pbdrv_bluetooth_tests[] = {
    PBIO_THREAD_TEST(test_bluetooth),
    PBIO_THREAD_TEST(test_bluetooth_nus),
    END_OF_TESTCASES
};
