// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup SystemBattery System: Battery Monitor
 * @{
 */

#ifndef _PBSYS_BATTERY_H_
#define _PBSYS_BATTERY_H_

#include <stdbool.h>

void pbsys_battery_init(void);
void pbsys_battery_poll(void);
bool pbsys_battery_is_full(void);

#endif // _PBSYS_BATTERY_H_

/** @} */
