// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup SystemBattery System: Battery Monitor
 * @{
 */

#ifndef _PBSYS_BATTERY_H_
#define _PBSYS_BATTERY_H_

#include <stdbool.h>

#include <pbsys/config.h>

#if PBSYS_CONFIG_BATTERY

void pbsys_battery_init(void);
void pbsys_battery_poll(void);
bool pbsys_battery_is_full(void);

#else // PBSYS_CONFIG_BATTERY

static inline void pbsys_battery_init(void) {
}

static inline void pbsys_battery_poll(void) {
}

static inline bool pbsys_battery_is_full(void) {
    return false;
}

#endif // PBSYS_CONFIG_BATTERY

#endif // _PBSYS_BATTERY_H_

/** @} */
