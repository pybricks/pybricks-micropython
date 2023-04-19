// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#ifndef _TEST_PBIO_H_
#define _TEST_PBIO_H_

#include <stdint.h>

#include <pbio/button.h>
#include <pbio/int_math.h>

// Use this macro to define tests that _don't_ require a Contiki event loop
#define PBIO_TEST(name) \
    { #name, name, TT_FORK, NULL, NULL }

// Use this macro to define tests that _do_ require a Contiki event loop
#define PBIO_PT_THREAD_TEST(name) \
    { #name, pbio_test_run_thread, TT_FORK, &pbio_test_setup, name }

void pbio_test_run_thread(void *env);
extern struct testcase_setup_t pbio_test_setup;

// these can be used by tests that use the bluetooth driver

bool pbio_test_bluetooth_is_advertising_enabled(void);
bool pbio_test_bluetooth_is_connected(void);
void pbio_test_bluetooth_connect(void);
void pbio_test_bluetooth_enable_uart_service_notifications(void);
uint32_t pbio_test_bluetooth_get_uart_service_notification_count(void);
void pbio_test_bluetooth_send_uart_data(const uint8_t *data, uint32_t size);
void pbio_test_bluetooth_enable_pybricks_service_notifications(void);
uint32_t pbio_test_bluetooth_get_pybricks_service_notification_count(void);
void pbio_test_bluetooth_send_pybricks_command(const uint8_t *data, uint32_t size);

typedef enum {
    PBIO_TEST_BLUETOOTH_STATE_OFF,
    PBIO_TEST_BLUETOOTH_STATE_ON,
} pbio_test_bluetooth_control_state_t;

pbio_test_bluetooth_control_state_t pbio_test_bluetooth_get_control_state(void);

// these can be used by tests that consume a counter device
void pbio_test_counter_set_angle(int32_t rotations, int32_t millidegrees);
void pbio_test_counter_set_abs_angle(int32_t millidegrees);

// these can be used by tests like servo or drivebases
#define pbio_test_sleep_until(condition) \
    while (!(condition)) { \
        pbio_test_clock_tick(1); \
        PT_YIELD(pt); \
    }

#define pbio_test_sleep_ms(timer, duration) \
    timer_set((timer), (duration)); \
    while (!timer_expired(timer)) { \
        pbio_test_clock_tick(1); \
        PT_YIELD(pt); \
    }

#define pbio_test_int_is_close(value, target, tolerance) (pbio_int_math_abs((value) - (target)) <= (tolerance))

#endif // _TEST_PBIO_H_
