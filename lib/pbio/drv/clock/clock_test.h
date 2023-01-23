// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_CLOCK_TEST_H_
#define _INTERNAL_PBDRV_CLOCK_TEST_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_TEST

#include <stdint.h>

// extra clock function just for tests
void pbio_test_clock_tick(uint32_t ticks);

#endif // PBDRV_CONFIG_CLOCK_TEST

#endif // _INTERNAL_PBDRV_CLOCK_TEST_H_
