// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

/**
 * @addtogroup ioport Driver: Driver for setting modes for input/output ports.
 *                            Currently there is only the legodev mode.
 * @{
 */

#ifndef PBDRV_IOPORT_H
#define PBDRV_IOPORT_H

#include <pbdrv/config.h>
#include <stdbool.h>

#if PBDRV_CONFIG_IOPORT

/**
 * Enables or disables VCC on pin 4 of all ioports.
 *
 * @param [in]  enable        Whether to enable or disable power.
 */
void pbdrv_ioport_enable_vcc(bool enable);

/**
 * Sets port I/O pins to default state and powers off VCC.
 *
 * Some hubs have a quirk where VCC can't be turned off without causing the
 * hub to immediately power back on. The return value indicates if it is safe
 * to attempt to power off the hub or not.
 *
 * @return true if the hub cannot be powered off, false otherwise.
 */
bool pbdrv_ioport_power_off(void);

#else // PBDRV_CONFIG_IOPORT

static inline void pbdrv_ioport_enable_vcc(bool enable) {
}

static inline bool pbdrv_ioport_power_off(void) {
    return false;
}

#endif // PBDRV_CONFIG_IOPORT

#endif // PBDRV_IOPORT_H

/** @} */
