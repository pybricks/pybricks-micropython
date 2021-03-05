// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#ifndef _TEST_PBIO_H_
#define _TEST_PBIO_H_

#include <stdint.h>

#include <pbio/button.h>

// Use this macro to define tests that _don't_ require a Contiki event loop
#define PBIO_TEST(name) \
    { #name, name, TT_FORK, NULL, NULL }

// Use this macro to define tests that _do_ require a Contiki event loop
#define PBIO_PT_THREAD_TEST(name) \
    { #name, pbio_test_run_thread, TT_FORK, &pbio_test_setup, name }

void pbio_test_run_thread(void *env);
extern struct testcase_setup_t pbio_test_setup;

// this can be used by tests that consume the button driver
void pbio_test_button_set_pressed(pbio_button_flags_t flags);

// these can be used by tests that use the bluetooth driver

bool pbio_test_bluetooth_is_advertising_enabled(void);
bool pbio_test_bluetooth_is_connected(void);
void pbio_test_bluetooth_connect(void);
void pbio_test_bluetooth_enable_uart_service_notifications(void);
void pbio_test_bluetooth_send_uart_data(const uint8_t *data, uint32_t size);

typedef enum {
    PBIO_TEST_BLUETOOTH_STATE_OFF,
    PBIO_TEST_BLUETOOTH_STATE_ON,
} pbio_test_bluetooth_control_state_t;

pbio_test_bluetooth_control_state_t pbio_test_bluetooth_get_control_state(void);

// these can be used by tests that consume a counter device
void pbio_test_counter_set_count(int32_t count);
void pbio_test_counter_set_abs_count(int32_t count);
void pbio_test_counter_set_rate(int32_t rate);

#endif // _TEST_PBIO_H_
