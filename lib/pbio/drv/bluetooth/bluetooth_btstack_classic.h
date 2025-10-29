// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_CLASSIC_H_
#define _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_CLASSIC_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC

#include <btstack_chipset.h>
#include <btstack_control.h>
#include <btstack_uart_block.h>
#include <pbio/os.h>

void pbdrv_bluetooth_btstack_classic_run_loop_trigger(void);

typedef struct {
    const btstack_uart_block_t *(*uart_block_instance)(void);
    const btstack_chipset_t *(*chipset_instance)(void);
    const btstack_control_t *(*control_instance)(void);
} pbdrv_bluetooth_btstack_platform_data_t;

// defined in platform.c
extern const pbdrv_bluetooth_btstack_platform_data_t pbdrv_bluetooth_btstack_platform_data;

// Main bluetooth process thread.
pbio_error_t pbdrv_bluetooth_process_thread(pbio_os_state_t *state, void *context);

#endif // PBDRV_CONFIG_BLUETOOTH_BTSTACK_CLASSIC

#endif // _INTERNAL_PBDRV_BLUETOOTH_BTSTACK_CLASSIC_H_
