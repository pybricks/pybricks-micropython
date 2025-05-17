// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_HOST

#include <pbdrv/usb.h>

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

/**
 * Transmits data over Bluetooth and USB.
 *
 * Should be called in a loop with the same arguments until it no longer
 * returns ::PBIO_ERROR_AGAIN.
 *
 * @param data  [in]   The data to transmit.
 * @param size  [in]   The size of the data to transmit.
 * @return             ::PBIO_ERROR_AGAIN if the data is still being transmitted
 *                     ::PBIO_SUCCESS if complete or failed.
 */
pbio_error_t pbsys_host_tx(const uint8_t *data, uint32_t size) {

    static bool transmitting = false;
    static uint32_t tx_done_ble;
    static uint32_t tx_done_usb;

    if (!transmitting) {
        tx_done_ble = 0;
        tx_done_usb = 0;
        transmitting = true;
    }

    pbio_error_t err_ble = PBIO_SUCCESS;
    pbio_error_t err_usb = PBIO_SUCCESS;
    uint32_t size_now;

    if (tx_done_ble < size) {
        size_now = size - tx_done_ble;
        err_ble = pbsys_bluetooth_tx(data + tx_done_ble, &size_now);
        tx_done_ble += size_now;
    }

    if (tx_done_usb < size) {
        size_now = size - tx_done_usb;
        err_usb = pbdrv_usb_stdout_tx(data + tx_done_usb, &size_now);
        tx_done_usb += size_now;
    }

    // Keep waiting as long as at least has not completed or errored.
    if (err_ble == PBIO_ERROR_AGAIN || err_usb == PBIO_ERROR_AGAIN) {
        return PBIO_ERROR_AGAIN;
    }

    // Both of them are either complete or failed. The caller of this function
    // does not currently raise errors, so we just return success.
    transmitting = false;
    return PBIO_SUCCESS;
}

bool pbsys_host_tx_is_idle(void) {
    return pbsys_bluetooth_tx_is_idle();
}

#endif // PBSYS_CONFIG_HOST
