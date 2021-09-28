// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <pbdrv/clock.h>

void mp_hal_set_interrupt_char(int c);
#define mp_hal_ticks_ms pbdrv_clock_get_ms
#define mp_hal_ticks_us pbdrv_clock_get_us
#define mp_hal_ticks_cpu() 0
