// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

#ifndef _PBIO_BATTERY_H_
#define _PBIO_BATTERY_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>

/**
 * The value that cooresponds to 100% duty cycle.
 */
#define PBIO_BATTERY_MAX_DUTY 1000

#if PBIO_CONFIG_DCMOTOR

pbio_error_t pbio_battery_init(void);
pbio_error_t pbio_battery_update(void);
int32_t pbio_battery_get_average_voltage(void);
int32_t pbio_battery_get_duty_from_voltage(int32_t voltage);
int32_t pbio_battery_get_voltage_from_duty(int32_t duty);
int32_t pbio_battery_get_voltage_from_duty_pct(int32_t duty);

#else

static inline pbio_error_t pbio_battery_init(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_battery_update(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline int32_t pbio_battery_get_average_voltage(void) {
    return 7200;
}

static inline int32_t pbio_battery_get_duty_from_voltage(int32_t voltage) {
    return 0;
}

static inline int32_t pbio_battery_get_voltage_from_duty(int32_t duty) {
    return 0;
}

static inline int32_t pbio_battery_get_voltage_from_duty_pct(int32_t duty) {
    return 0;
}

#endif // PBIO_CONFIG_DCMOTOR

#endif // _PBIO_BATTERY_H_
