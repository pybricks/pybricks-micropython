// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_BATTERY

#include <inttypes.h>

#include <pbdrv/battery.h>
#include <pbio/battery.h>
#include <pbio/math.h>

// Slow moving average battery voltage.
static int32_t battery_voltage_avg_scaled;

// The average battery value is scaled up numerically
// to reduce rounding errors in the moving average.
#define SCALE (1024)

// Initializes average to first measurement.
pbio_error_t pbio_battery_init(void) {

    // Get battery voltage.
    uint16_t battery_voltage_now_mv;
    pbio_error_t err = pbdrv_battery_get_voltage_now(&battery_voltage_now_mv);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Initialize average voltage.
    battery_voltage_avg_scaled = (int32_t)battery_voltage_now_mv * SCALE;

    return PBIO_SUCCESS;
}

// Updates the average voltage.
pbio_error_t pbio_battery_update(void) {

    // Get battery voltage.
    uint16_t battery_voltage_now_mv;
    pbio_error_t err = pbdrv_battery_get_voltage_now(&battery_voltage_now_mv);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Update moving average.
    battery_voltage_avg_scaled = (battery_voltage_avg_scaled * 127 + ((int32_t)battery_voltage_now_mv) * SCALE) / 128;

    return PBIO_SUCCESS;
}

// Gets the moving average value.
int32_t pbio_battery_get_average_voltage(void) {
    return battery_voltage_avg_scaled / SCALE;
}

/**
 * Gets the duty cycle required to output the desired voltage given the
 * current battery voltage.
 *
 * @param [in]  voltage     The desired voltage in mV.
 * @return                  A duty cycle in the range negative ::PBIO_BATTERY_MAX_DUTY
 *                          to positive ::PBIO_BATTERY_MAX_DUTY.
 */
int32_t pbio_battery_get_duty_from_voltage(int32_t voltage) {
    // Calculate unbounded duty cycle value.
    int32_t duty_cycle = voltage * PBIO_BATTERY_MAX_DUTY / (battery_voltage_avg_scaled / SCALE);

    return pbio_math_clamp(duty_cycle, PBIO_BATTERY_MAX_DUTY);
}

/**
 * Gets the effective voltage resulting from @p duty given the current battery voltage.
 *
 * If @p duty it outside of the specified range, it will be clamped to that range.
 *
 * @param [in]  duty    The duty cycle in the range negative ::PBIO_BATTERY_MAX_DUTY
 *                      to positive ::PBIO_BATTERY_MAX_DUTY.
 * @return              The battery voltage in mV.
 */
int32_t pbio_battery_get_voltage_from_duty(int32_t duty) {
    duty = pbio_math_clamp(duty, PBIO_BATTERY_MAX_DUTY);
    return duty * (battery_voltage_avg_scaled / SCALE) / PBIO_BATTERY_MAX_DUTY;
}

/**
 * Gets the effective voltage resulting from @p duty given the current battery voltage.
 *
 * If @p duty it outside of the specified range, it will be clamped to that range.
 *
 * @param [in]  duty    The duty cycle percentage in the range -100 to 100.
 * @return              The battery voltage in mV.
 */
int32_t pbio_battery_get_voltage_from_duty_pct(int32_t duty) {
    duty = pbio_math_clamp(duty, 100);
    return duty * (battery_voltage_avg_scaled / SCALE) / 100;
}

#endif // PBIO_CONFIG_BATTERY
