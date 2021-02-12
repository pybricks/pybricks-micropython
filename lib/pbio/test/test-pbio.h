// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#ifndef _TEST_PBIO_H_
#define _TEST_PBIO_H_

#include <stdint.h>

#include <pbio/button.h>

// this can be used by tests that consume the button driver
void pbio_test_button_set_pressed(pbio_button_flags_t flags);

// these can be used by tests that use the bluetooth driver

bool pbio_test_bluetooth_is_advertising_enabled(void);
void pbio_test_bluetooth_connect(void);

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
