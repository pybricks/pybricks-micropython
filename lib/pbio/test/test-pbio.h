// SPDX-License-Identifier: MIT
// Copyright 2020 The Pybricks Authors

#ifndef _TEST_PBIO_H_
#define _TEST_PBIO_H_

#include <stdint.h>

// these can be used by tests that consume a counter device
void pbio_test_counter_set_count(int32_t count);
void pbio_test_counter_set_abs_count(int32_t count);
void pbio_test_counter_set_rate(int32_t rate);

#endif // _TEST_PBIO_H_
