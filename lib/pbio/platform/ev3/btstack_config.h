// SPDX-License-Identifier: MIT
// Copyright (c) 2025-2026 The Pybricks Authors

// BlueKitchen BTstack config.

#ifndef _PLATFORM_EV3_BTSTACK_CONFIG_H_
#define _PLATFORM_EV3_BTSTACK_CONFIG_H_

// The CC2560X can hold more links than the baseband ceiling of 7 slaves, so
// the derived count is what limits this, not the controller.
#include <drv/bluetooth/bluetooth_btstack_config.h>

#endif // _PLATFORM_EV3_BTSTACK_CONFIG_H_
