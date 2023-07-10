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

#else // PBDRV_CONFIG_IOPORT

static inline void pbdrv_ioport_enable_vcc(bool enable) {
}

#endif // PBDRV_CONFIG_IOPORT

#endif // PBDRV_IOPORT_H

/** @} */
