// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <stdio.h>

#include <contiki.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/battery.h>
#include <test-pbio.h>

// pbdrv_battery_get_voltage_now() always returns this value
#define TEST_BATTERY_VOLTAGE 7200

static void test_battery_voltage_to_duty(void *env) {
    pbio_battery_init();
    tt_want_int_op(pbio_battery_get_duty_from_voltage(TEST_BATTERY_VOLTAGE), ==, PBIO_BATTERY_MAX_DUTY);
    tt_want_int_op(pbio_battery_get_duty_from_voltage(TEST_BATTERY_VOLTAGE + 1000), ==, PBIO_BATTERY_MAX_DUTY);
    tt_want_int_op(pbio_battery_get_duty_from_voltage(TEST_BATTERY_VOLTAGE / 2), ==, PBIO_BATTERY_MAX_DUTY / 2);
    tt_want_int_op(pbio_battery_get_duty_from_voltage(-TEST_BATTERY_VOLTAGE), ==, -PBIO_BATTERY_MAX_DUTY);
}

static void test_battery_voltage_from_duty(void *env) {
    pbio_battery_init();
    tt_want_int_op(pbio_battery_get_voltage_from_duty(PBIO_BATTERY_MAX_DUTY), ==, TEST_BATTERY_VOLTAGE);
    tt_want_int_op(pbio_battery_get_voltage_from_duty(PBIO_BATTERY_MAX_DUTY + 1000), ==, TEST_BATTERY_VOLTAGE);
    tt_want_int_op(pbio_battery_get_voltage_from_duty(PBIO_BATTERY_MAX_DUTY / 2), ==, TEST_BATTERY_VOLTAGE / 2);
    tt_want_int_op(pbio_battery_get_voltage_from_duty(-PBIO_BATTERY_MAX_DUTY), ==, -TEST_BATTERY_VOLTAGE);
}

static void test_battery_voltage_from_duty_pct(void *env) {
    pbio_battery_init();
    tt_want_int_op(pbio_battery_get_voltage_from_duty_pct(100), ==, TEST_BATTERY_VOLTAGE);
    tt_want_int_op(pbio_battery_get_voltage_from_duty_pct(150), ==, TEST_BATTERY_VOLTAGE);
    tt_want_int_op(pbio_battery_get_voltage_from_duty_pct(50), ==, TEST_BATTERY_VOLTAGE / 2);
    tt_want_int_op(pbio_battery_get_voltage_from_duty_pct(-100), ==, -TEST_BATTERY_VOLTAGE);
}

struct testcase_t pbio_battery_tests[] = {
    PBIO_TEST(test_battery_voltage_to_duty),
    PBIO_TEST(test_battery_voltage_from_duty),
    PBIO_TEST(test_battery_voltage_from_duty_pct),
    END_OF_TESTCASES
};
