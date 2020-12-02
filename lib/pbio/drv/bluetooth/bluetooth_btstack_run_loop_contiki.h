// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Contiki run loop integration for BlueKitchen BTStack.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_RUN_LOOP_CONTIKI_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_RUN_LOOP_CONTIKI_H_

#include <btstack_run_loop.h>

const btstack_run_loop_t *pbdrv_bluetooth_btstack_run_loop_contiki_get_instance(void);
void pbdrv_bluetooth_btstack_run_loop_contiki_trigger();

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_RUN_LOOP_CONTIKI_H_
