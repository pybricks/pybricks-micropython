// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <pbdrv/clock.h>

void mp_hal_set_interrupt_char(int c);
#define mp_hal_ticks_ms pbdrv_clock_get_ms
#define mp_hal_ticks_us pbdrv_clock_get_us
#define mp_hal_ticks_cpu() 0
#define mp_hal_delay_us pbdrv_clock_delay_us
void pb_stack_get_info(char **sstack, char **estack);

// Platform-specific code to run on completing the poll hook.
void pb_event_poll_hook_leave(void);
