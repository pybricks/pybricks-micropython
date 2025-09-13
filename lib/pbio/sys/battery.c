// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Provides battery status indication and shutdown on low battery.

// TODO: need to handle high battery current
// TODO: need to handle battery pack switch and Li-ion batteries for Technic Hub and NXT

#include <pbdrv/battery.h>
#include <pbio/battery.h>
#include <pbio/os.h>
#include <pbdrv/charger.h>
#include <pbdrv/config.h>
#include <pbdrv/clock.h>
#include <pbdrv/usb.h>
#include <pbsys/config.h>
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

#if PBSYS_CONFIG_BATTERY_TEMP_ESTIMATION

#include <math.h>

#define PBSYS_BATTERY_TEMP_TIMER_PERIOD_MS 400

static pbio_os_timer_t pbsys_battery_temp_timer;

extern float pbsys_battery_temp_update(float V_bat, float I_bat);

#endif // PBSYS_CONFIG_BATTERY_TEMP_ESTIMATION

void pbsys_battery_init(void) {
    #if PBSYS_CONFIG_BATTERY_TEMP_ESTIMATION
    pbio_os_timer_set(&pbsys_battery_temp_timer, PBSYS_BATTERY_TEMP_TIMER_PERIOD_MS);
    #endif
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

    // Shut down on low voltage so we don't damage rechargeable batteries.
    if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BATTERY_LOW_VOLTAGE_SHUTDOWN, true, 3000)) {
        pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
    }

    #if PBSYS_CONFIG_BATTERY_TEMP_ESTIMATION
    if (!is_liion && pbio_os_timer_is_expired(&pbsys_battery_temp_timer)) {
        pbio_os_timer_extend(&pbsys_battery_temp_timer);

        uint16_t voltage_mv;
        uint16_t current_ma;
        if (pbdrv_battery_get_voltage_now(&voltage_mv) == PBIO_SUCCESS &&
            pbdrv_battery_get_current_now(&current_ma) == PBIO_SUCCESS) {

            static float old_temp;
            float new_temp = pbsys_battery_temp_update(voltage_mv / 1000.0f, current_ma / 1000.0f);
            if (fabsf(new_temp - old_temp) > 0.1f) {
                old_temp = new_temp;
            }

            const float high_temp_warning = 25.0f;
            const float high_temp_critical = 30.0f;

            if (old_temp >= high_temp_warning) {
                pbsys_status_set(PBIO_PYBRICKS_STATUS_BATTERY_HIGH_TEMP_WARNING);
            } else if (old_temp < high_temp_warning) {
                pbsys_status_clear(PBIO_PYBRICKS_STATUS_BATTERY_HIGH_TEMP_WARNING);
            }

            if (old_temp >= high_temp_critical) {
                pbsys_status_set(PBIO_PYBRICKS_STATUS_BATTERY_HIGH_TEMP_SHUTDOWN);
            } else if (old_temp < high_temp_critical) {
                pbsys_status_clear(PBIO_PYBRICKS_STATUS_BATTERY_HIGH_TEMP_SHUTDOWN);
            }

            // Shut down on high temperature so we don't damage AAA batteries.
            if (pbsys_status_test_debounce(PBIO_PYBRICKS_STATUS_BATTERY_HIGH_TEMP_SHUTDOWN, true, 3000)) {
                pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);
            }
        }
    }
    #endif
}

/**
 * Tests if the battery is "full".
 *
 * This is only valid on hubs with a built-in battery charger.
 */
bool pbsys_battery_is_full(void) {
    return pbio_battery_get_average_voltage() >= LIION_FULL_MV;
}
