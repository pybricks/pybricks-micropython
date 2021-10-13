// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_DCMOTOR

#include <inttypes.h>

#include <pbdrv/battery.h>
#include <pbdrv/motor.h>

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

// Gets the duty cycle required to output the desired voltage.
int32_t pbio_battery_get_duty_from_voltage(int32_t voltage) {
    // Calculate unbounded duty cycle value.
    int32_t duty_cycle = voltage * PBDRV_MAX_DUTY / (battery_voltage_avg_scaled / SCALE);

    // Return bounded duty cycle value.
    if (duty_cycle > PBDRV_MAX_DUTY) {
        return PBDRV_MAX_DUTY;
    }
    if (duty_cycle < -PBDRV_MAX_DUTY) {
        return -PBDRV_MAX_DUTY;
    }
    return duty_cycle;
}

// Gets the voltage resulting from the given duty cycle.
int32_t pbio_battery_get_voltage_from_duty(int32_t duty) {
    return duty * (battery_voltage_avg_scaled / SCALE) / PBDRV_MAX_DUTY;
}

#endif // PBIO_CONFIG_DCMOTOR
