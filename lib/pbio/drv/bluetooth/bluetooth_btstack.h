// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Pybricks platform data for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_

#include <btstack_chipset.h>
#include <btstack_control.h>
#include <btstack_uart_block.h>

void pbdrv_bluetooth_btstack_run_loop_trigger(void);

/**
 * Hook called when BTstack reads the local version information.
 *
 * This is called _after_ hci_set_chipset but _before_ the init script is sent
 * over the wire, so this can be used to dynamically select the init script.
 */
void pbdrv_bluetooth_btstack_set_chipset(uint16_t lmp_pal_subversion);

typedef struct {
    const hci_transport_t *(*transport_instance)(void);
    const void *(*transport_config)(void);
    const btstack_chipset_t *(*chipset_instance)(void);
    const btstack_control_t *(*control_instance)(void);
    const uint8_t *er_key;
    const uint8_t *ir_key;
} pbdrv_bluetooth_btstack_platform_data_t;

// defined in platform.c
extern const pbdrv_bluetooth_btstack_platform_data_t pbdrv_bluetooth_btstack_platform_data;

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_H_
