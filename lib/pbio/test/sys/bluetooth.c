// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdio.h>

#include <btstack.h>
#include <contiki.h>
#include <tinytest_macros.h>
#include <tinytest.h>

#include <pbio/util.h>
#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <test-pbio.h>

static PT_THREAD(test_bluetooth(struct pt *pt)) {
    PT_BEGIN(pt);

    pbsys_bluetooth_init();

    // power should be initialized to off
    tt_want_uint_op(pbio_test_bluetooth_get_control_state(), ==, PBIO_TEST_BLUETOOTH_STATE_OFF);

    // wait for the power on delay
    PT_WAIT_UNTIL(pt, ({
        pbio_test_clock_tick(1);
        pbio_test_bluetooth_get_control_state() == PBIO_TEST_BLUETOOTH_STATE_ON;
    }));

    PT_WAIT_UNTIL(pt, ({
        pbio_test_clock_tick(1);
        pbio_test_bluetooth_is_advertising_enabled();
    }));

    pbio_test_bluetooth_connect();

    PT_WAIT_UNTIL(pt, ({
        pbio_test_clock_tick(1);
        pbio_test_bluetooth_is_connected();
    }));

    // TODO: enable pybricks service notifications and do concurrent pybricks service and uart service calls

    pbio_test_bluetooth_enable_uart_service_notifications();

    static const char *test_data_1 = "test\n";
    static const char *test_data_2 = "test2\n";
    uint32_t size;

    PT_WAIT_UNTIL(pt, ({
        pbio_test_clock_tick(1);
        size = strlen(test_data_1);
        pbsys_bluetooth_tx((const uint8_t *)test_data_1, &size) == PBIO_SUCCESS;
    }));

    tt_want_uint_op(size, ==, strlen(test_data_1));

    // yielding here should allow the pbsys bluetooth process to run and queue
    // the data in the uart buffer
    PT_YIELD(pt);

    // this next data should get pushed in the UART buffer but wait until the
    // previous request is finished before sending
    PT_WAIT_UNTIL(pt, ({
        pbio_test_clock_tick(1);
        size = strlen(test_data_2);
        pbsys_bluetooth_tx((const uint8_t *)test_data_2, &size) == PBIO_SUCCESS;
    }));

    tt_want_uint_op(size, ==, strlen(test_data_2));

    PT_YIELD(pt);

    static const char *test_data_3 = "test3\n";
    size = strlen(test_data_3);
    pbio_test_bluetooth_send_uart_data((const uint8_t *)test_data_3, size);

    static uint8_t rx_data[20];

    PT_WAIT_UNTIL(pt, ({
        pbio_test_clock_tick(1);
        size = PBIO_ARRAY_SIZE(rx_data);
        pbsys_bluetooth_rx(rx_data, &size) == PBIO_SUCCESS;
    }));

    tt_want_uint_op(size, ==, strlen(test_data_3));
    tt_want_int_op(strncmp(test_data_3, (const char *)rx_data, size), ==, 0);

    // enabling notifications on Pybricks command characteristic should send
    // a notification right away if status is non-zero
    pbsys_status_set(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING);
    pbio_test_bluetooth_enable_pybricks_service_notifications();

    static uint32_t count;
    count = pbio_test_bluetooth_get_pybricks_service_notification_count();

    PT_WAIT_UNTIL(pt, ({
        pbio_test_clock_tick(1);
        pbio_test_bluetooth_get_pybricks_service_notification_count() != count;
    }));

    PT_END(pt);
}

struct testcase_t pbsys_bluetooth_tests[] = {
    PBIO_PT_THREAD_TEST(test_bluetooth),
    END_OF_TESTCASES
};
