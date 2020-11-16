// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Hooks for unit tests.

#ifndef _INTERNAL_PBDRV_COUNTER_TEST_H_
#define _INTERNAL_PBDRV_COUNTER_TEST_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_TEST

#include <pbdrv/counter.h>

void pbdrv_counter_test_init(pbdrv_counter_dev_t *devs);

#else // PBDRV_CONFIG_COUNTER_TEST

#define pbdrv_counter_test_init(dev)

#endif // PBDRV_CONFIG_COUNTER_TEST

#endif // _INTERNAL_PBDRV_COUNTER_TEST_H_
