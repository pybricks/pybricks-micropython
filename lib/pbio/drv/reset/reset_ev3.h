// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// The EV3 requires a GPIO pin to be set in order to stay powered on.
// We want to be able to do this as early as possible.

#ifndef _INTERNAL_PBDRV_RESET_EV3_H_
#define _INTERNAL_PBDRV_RESET_EV3_H_

void pbdrv_reset_ev3_early_init(void);

#endif // _INTERNAL_PBDRV_RESET_EV3_H_
