// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 LEGO System A/S

#include <pbio/config.h>

#if PBIO_CONFIG_BATTERY

#include <inttypes.h>

#include <pbdrv/battery.h>
#include <pbio/battery.h>
#include <pbio/int_math.h>
#include <pbio/os.h>

// Slow moving average battery voltage.
static int32_t battery_voltage_avg_scaled;

// The average battery value is scaled up numerically
// to reduce rounding errors in the moving average.
#define SCALE (1024)

/**
 * Gets the moving average battery voltage.
 *
 * @return                  The voltage in mV.
 */
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

    return pbio_int_math_clamp(duty_cycle, PBIO_BATTERY_MAX_DUTY);
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
    duty = pbio_int_math_clamp(duty, PBIO_BATTERY_MAX_DUTY);
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
    duty = pbio_int_math_clamp(duty, 100);
    return duty * (battery_voltage_avg_scaled / SCALE) / 100;
}

static pbio_os_process_t pbio_battery_process;

static pbio_error_t pbio_battery_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    pbio_os_timer_set(&timer, PBIO_CONFIG_CONTROL_LOOP_TIME_MS);

    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&timer));

        uint16_t battery_voltage_now_mv;
        pbdrv_battery_get_voltage_now(&battery_voltage_now_mv);
        // Returned error is ignored.

        // Update moving average.
        battery_voltage_avg_scaled = (battery_voltage_avg_scaled * 127 + ((int32_t)battery_voltage_now_mv) * SCALE) / 128;

        pbio_os_timer_extend(&timer);
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

/**
 * Initializes battery voltage state to first measurement.
 */
void pbio_battery_init(void) {

    // Get battery voltage.
    uint16_t battery_voltage_now_mv;
    pbdrv_battery_get_voltage_now(&battery_voltage_now_mv);
    // Returned error is ignored.

    // Initialize average upscaled voltage.
    battery_voltage_avg_scaled = (int32_t)battery_voltage_now_mv * SCALE;

    pbio_os_process_start(&pbio_battery_process, pbio_battery_process_thread, NULL);
}

#endif // PBIO_CONFIG_BATTERY
