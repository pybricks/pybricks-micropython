// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_SIMULATION

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "usb.h"
#include <pbdrv/usb.h>

#include <pbio/error.h>
#include <pbio/os.h>

void pbdrv_usb_set_receive_handler(pbdrv_usb_receive_handler_t handler) {
}

void pbdrv_usb_schedule_status_update(const uint8_t *status_msg) {
}

pbio_error_t pbdrv_usb_stdout_tx(const uint8_t *data, uint32_t *size) {
    return PBIO_SUCCESS;
}

uint32_t pbdrv_usb_stdout_tx_available(void) {
    return UINT32_MAX;
}

bool pbdrv_usb_stdout_tx_is_idle(void) {
    return true;
}

bool pbdrv_usb_connection_is_active(void) {
    return false;
}

void pbdrv_usb_init(void) {
    pbdrv_usb_init_device();
}

void pbdrv_usb_deinit(void) {
}

#endif // PBDRV_CONFIG_USB_SIMULATION
