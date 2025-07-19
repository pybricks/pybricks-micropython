// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_HOST

#include <lwrb/lwrb.h>

#include <pbdrv/usb.h>
#include <pbsys/host.h>

#include "bluetooth.h"

static pbsys_host_stdin_event_callback_t pbsys_host_stdin_event_callback;
static lwrb_t pbsys_host_stdin_ring_buf;

void pbsys_host_init(void) {
    static uint8_t stdin_buf[PBSYS_CONFIG_HOST_STDIN_BUF_SIZE];
    lwrb_init(&pbsys_host_stdin_ring_buf, stdin_buf, PBIO_ARRAY_SIZE(stdin_buf));
    pbsys_bluetooth_init();
}

// Publisher APIs. Pybricks Profile connections call these to push data to
// a common stdin buffer.

/**
 * Gets the number of bytes currently free for writing in stdin.
 * @return              The number of bytes.
 */
uint32_t pbsys_host_stdin_get_free(void) {
    return lwrb_get_free(&pbsys_host_stdin_ring_buf);
}

/**
 * Writes data to the stdin buffer.
 *
 * This does not currently return the number of bytes written, so first call
 * pbsys_bluetooth_stdin_get_free() to ensure enough free space.
 *
 * @param [in]  data    The data to write to the stdin buffer.
 * @param [in]  size    The size of @p data in bytes.
 */
void pbsys_host_stdin_write(const uint8_t *data, uint32_t size) {
    if (pbsys_host_stdin_event_callback) {
        // If there is a callback hook, we have to process things one byte at
        // a time. This is needed, e.g. by Micropython to handle Ctrl-C.
        for (uint32_t i = 0; i < size; i++) {
            if (!pbsys_host_stdin_event_callback(data[i])) {
                lwrb_write(&pbsys_host_stdin_ring_buf, &data[i], 1);
            }
        }
    } else {
        lwrb_write(&pbsys_host_stdin_ring_buf, data, size);
    }
}

// Consumer APIs. User-facing code calls these to read data from stdin.

/**
 * Sets the host stdin callback function.
 * @param callback  [in]    The callback or NULL.
 */
void pbsys_host_stdin_set_callback(pbsys_host_stdin_event_callback_t callback) {
    pbsys_host_stdin_event_callback = callback;
}

/**
 * Flushes data from the stdin buffer without reading it.
 */
void pbsys_host_stdin_flush(void) {
    lwrb_reset(&pbsys_host_stdin_ring_buf);
}

/**
 * Gets the number of bytes currently available to be read from the host stdin buffer.
 * @return              The number of bytes.
 */
uint32_t pbsys_host_stdin_get_available(void) {
    return lwrb_get_full(&pbsys_host_stdin_ring_buf);
}

/**
 * Reads data from the stdin buffer.
 * @param data  [in]        A buffer to receive a copy of the data.
 * @param size  [in, out]   The number of bytes to read (@p data must be at least
 *                          this big). After return @p size contains the number
 *                          of bytes actually read.
 * @return                  ::PBIO_SUCCESS if @p data was read, ::PBIO_ERROR_AGAIN
 *                          if @p data could not be read at this time (i.e. buffer
 *                          is empty), ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support Bluetooth.
 */
pbio_error_t pbsys_host_stdin_read(uint8_t *data, uint32_t *size) {
    if ((*size = lwrb_read(&pbsys_host_stdin_ring_buf, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
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
