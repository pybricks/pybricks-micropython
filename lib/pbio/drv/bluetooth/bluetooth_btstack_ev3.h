// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// EV3 for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_EV3_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_EV3_H_

#include <btstack.h>

#include "bluetooth_btstack.h"

#include <pbdrv/gpio.h>

// Defined in platform/ev3.
extern const pbdrv_bluetooth_btstack_chipset_info_t cc2560_info;
extern const pbdrv_bluetooth_btstack_chipset_info_t cc2560a_info;

const btstack_control_t *pbdrv_bluetooth_btstack_ev3_control_instance(void);

const hci_transport_t *pbdrv_bluetooth_btstack_ev3_transport_instance(void);

const void *pbdrv_bluetooth_btstack_ev3_transport_config(void);

void pbdrv_bluetooth_btstack_ev3_handle_rx_complete(void);
void pbdrv_bluetooth_btstack_ev3_handle_tx_complete(void);

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_EV3_H_
