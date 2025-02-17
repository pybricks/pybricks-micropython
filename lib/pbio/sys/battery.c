// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

// Provides battery status indication and shutdown on low battery.

// TODO: need to handle high battery current
// TODO: need to handle battery pack switch and Li-ion batteries for Technic Hub and NXT

#include <pbdrv/battery.h>
#include <pbio/battery.h>
#include <pbdrv/charger.h>
#include <pbdrv/config.h>
#include <pbdrv/clock.h>
#include <pbdrv/usb.h>
#include <pbsys/status.h>

// These values are for Alkaline (AA/AAA) batteries
#define BATTERY_OK_MV           6000    // 1.0V per cell
#define BATTERY_LOW_MV          5400    // 0.9V per cell
#define BATTERY_CRITICAL_MV     4800    // 0.8V per cell

// These values are for LEGO rechargeable battery packs
#define LIION_FULL_MV           8190    // 4.095V per cell
#define LIION_OK_MV             7200    // 3.6V per cell
#define LIION_LOW_MV            6800    // 3.4V per cell
#define LIION_CRITICAL_MV       6000    // 3.0V per cell

void pbsys_battery_init(void) {
}

/**
 * Polls the battery.
 *
 * This is called periodically to update the current battery state.
 */
void pbsys_battery_poll(void) {

    pbdrv_battery_type_t type;
    bool is_liion = pbdrv_battery_get_type(&type) == PBIO_SUCCESS && type == PBDRV_BATTERY_TYPE_LIION;

    uint32_t battery_critical_mv = is_liion ? LIION_CRITICAL_MV : BATTERY_CRITICAL_MV;
    uint32_t battery_low_mv = is_liion ? LIION_LOW_MV : BATTERY_LOW_MV;
    uint32_t battery_ok_mv = is_liion ? LIION_OK_MV : BATTERY_OK_MV;

    uint32_t avg_battery_voltage = pbio_battery_get_average_voltage();

    if (avg_battery_voltage <= battery_critical_mv) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN);
    } else if (avg_battery_voltage >= battery_low_mv) {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN);
    }

    if (avg_battery_voltage <= battery_low_mv) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING);
    } else if (avg_battery_voltage >= battery_ok_mv) {
        pbsys_status_clear(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_WARNING);
    }
}

/**
 * Tests if the battery is "full".
 *
 * This is only valid on hubs with a built-in battery charger.
 */
bool pbsys_battery_is_full(void) {
    return pbio_battery_get_average_voltage() >= LIION_FULL_MV;
}
