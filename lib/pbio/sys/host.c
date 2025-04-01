// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_HOST

#include <pbsys/host.h>

#include "bluetooth.h"

void pbsys_host_init(void) {
    pbsys_bluetooth_init();
}

void pbsys_host_rx_set_callback(pbsys_host_stdin_event_callback_t callback) {
    pbsys_bluetooth_rx_set_callback(callback);
}

void pbsys_host_rx_flush(void) {
    pbsys_bluetooth_rx_flush();
}

uint32_t pbsys_host_rx_get_available(void) {
    return pbsys_bluetooth_rx_get_available();
}

uint32_t pbsys_host_rx_get_free(void) {
    return pbsys_bluetooth_rx_get_free();
}

void pbsys_host_rx_write(const uint8_t *data, uint32_t size) {
    pbsys_bluetooth_rx_write(data, size);
}

pbio_error_t pbsys_host_rx(uint8_t *data, uint32_t *size) {
    return pbsys_bluetooth_rx(data, size);
}

pbio_error_t pbsys_host_tx(const uint8_t *data, uint32_t *size) {
    return pbsys_bluetooth_tx(data, size);
}

bool pbsys_host_tx_is_idle(void) {
    return pbsys_bluetooth_tx_is_idle();
}

#endif // PBSYS_CONFIG_HOST
