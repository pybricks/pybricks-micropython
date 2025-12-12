// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_RPROC_NXT_H_
#define _INTERNAL_PBDRV_RPROC_NXT_H_

#include <pbio/button.h>
#include <pbio/error.h>

#include <pbdrv/reset.h>

void pbdrv_rproc_nxt_reset_host(pbdrv_reset_action_t action) __attribute__((noreturn));

pbio_error_t pbdrv_rproc_nxt_set_duty_cycle(uint8_t index, int32_t duty_cycle_percent, bool slow_decay);

typedef enum {
    /** Turn off sensor power. */
    PBDRV_RPROC_NXT_SENSOR_POWER_OFF = 0,
    /** Pulse sensor power for active RCX sensors. */
    PBDRV_RPROC_NXT_SENSOR_POWER_PULSED = 0x01,
    /** Turn on sensor power. */
    PBDRV_RPROC_NXT_SENSOR_POWER_ON = 0x11,
} pbdrv_rproc_nxt_sensor_power_t;

pbio_error_t pbdrv_rproc_nxt_set_sensor_power(uint8_t index, pbdrv_rproc_nxt_sensor_power_t power_type);

pbio_button_flags_t pbdrv_rproc_nxt_get_button_pressed(void);

bool pbdrv_rproc_nxt_get_battery_info(uint16_t *voltage);

#endif // _INTERNAL_PBDRV_RPROC_NXT_H_
