// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RPROC_NXT_H_
#define _INTERNAL_PBDRV_RPROC_NXT_H_

#include <pbio/button.h>
#include <pbio/error.h>

#include <pbdrv/reset.h>

void pbdrv_rproc_nxt_reset_host(pbdrv_reset_action_t action) __attribute__((noreturn));

pbio_error_t pbdrv_rproc_nxt_set_duty_cycle(uint8_t index, int32_t duty_cycle_percent, bool slow_decay);

pbio_button_flags_t pbdrv_rproc_nxt_get_button_pressed(void);

bool pbdrv_rproc_nxt_get_battery_info(uint16_t *voltage);

#endif // _INTERNAL_PBDRV_RPROC_NXT_H_
