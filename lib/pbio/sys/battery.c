// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Provides battery status indication and shutdown on low battery.

// TODO: need to handle high battery current
// TOOD: need to handle Li-ion batteries and charger for SPIKE Prime
// TODO: need to handle battery pack switch and Li-ion batteries for Control+ and NXT

#include <contiki.h>

#include <pbdrv/battery.h>

#include <pbsys/status.h>
#include <pbsys/sys.h>

// period over which the battery voltage is averaged (in milliseconds)
#define BATTERY_PERIOD_MS       2500

// These values are for Alkaline (AA/AAA) batteries
#define BATTERY_OK_MV           6000    // 1.0V per cell
#define BATTERY_LOW_MV          5400    // 0.9V per cell
#define BATTERY_CRITICAL_MV     4800    // 0.8V per cell

static uint16_t avg_battery_voltage;
static clock_time_t prev_poll_time;

/**
 * Initializes the system battery moitor.
 */
void pbsys_battery_init() {
    pbdrv_battery_get_voltage_now(&avg_battery_voltage);
    // This is mainly for Control+ hub. It seems that the first battery voltage
    // read is always low and causes the hub to shut down because of low battery
    // voltage even though the batter isn't that low.
    if (avg_battery_voltage < BATTERY_CRITICAL_MV) {
        avg_battery_voltage = BATTERY_CRITICAL_MV;
    }
    prev_poll_time = clock_time();
}

/**
 * Polls the battery.
 *
 * This is called periodically to update the current battery state.
 */
void pbsys_battery_poll() {
    clock_time_t now;
    uint32_t poll_interval;
    uint16_t battery_voltage;

    now = clock_time();
    poll_interval = clock_to_msec(now - prev_poll_time);
    prev_poll_time = now;

    pbdrv_battery_get_voltage_now(&battery_voltage);

    avg_battery_voltage = (avg_battery_voltage * (BATTERY_PERIOD_MS - poll_interval)
        + battery_voltage * poll_interval) / BATTERY_PERIOD_MS;

    // Shut down on low voltage so we don't damage rechargeable batteries
    if (avg_battery_voltage <= BATTERY_CRITICAL_MV) {
        // TODO: set PBSYS_STATUS_POWER_DOWN_PENDING and delay power off to give
        // some warning that power down is imminent.
        pbsys_power_off();
    }

    if (avg_battery_voltage <= BATTERY_LOW_MV) {
        pbsys_status_set_flag(PBSYS_STATUS_BATTERY_LOW_VOLTAGE);
    } else if (avg_battery_voltage >= BATTERY_OK_MV) {
        pbsys_status_clear_flag(PBSYS_STATUS_BATTERY_LOW_VOLTAGE);
    }
}
