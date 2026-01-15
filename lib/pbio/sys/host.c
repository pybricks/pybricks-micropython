// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_HOST

#include <lwrb/lwrb.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/usb.h>

#include <pbsys/command.h>
#include <pbsys/host.h>

#define BLE_ONLY (PBDRV_CONFIG_BLUETOOTH && (!PBDRV_CONFIG_USB || PBDRV_CONFIG_USB_CHARGE_ONLY))
#define USB_ONLY (!PBDRV_CONFIG_BLUETOOTH && PBDRV_CONFIG_USB && !PBDRV_CONFIG_USB_CHARGE_ONLY)
#define BLE_AND_USB (PBDRV_CONFIG_BLUETOOTH && PBDRV_CONFIG_USB && !PBDRV_CONFIG_USB_CHARGE_ONLY)

static pbsys_host_stdin_event_callback_t pbsys_host_stdin_event_callback;
static lwrb_t pbsys_host_stdin_ring_buf;

void pbsys_host_init(void) {
    static uint8_t stdin_buf[PBSYS_CONFIG_HOST_STDIN_BUF_SIZE];
    lwrb_init(&pbsys_host_stdin_ring_buf, stdin_buf, PBIO_ARRAY_SIZE(stdin_buf));

    pbdrv_bluetooth_set_receive_handler(pbsys_command);
    pbdrv_usb_set_receive_handler(pbsys_command);
}

/**
 * Schedules sending a status update to connected hosts.
 *
 * The data length is always ::PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE.
 */
void pbsys_host_schedule_status_update(const uint8_t *buf) {
    pbdrv_bluetooth_schedule_status_update(buf);
    pbdrv_usb_schedule_status_update(buf);
}

/**
 * Tests if the hub is connected to the host with BLE or USB.
 *
 * Connected implies an active connection to a Pybricks app, not just
 * physically plugged in.
 *
 * @return              @c true if connection is active, else @c false.
 */
bool pbsys_host_is_connected(void) {
    return pbdrv_bluetooth_host_is_connected() ||
           pbdrv_usb_connection_is_active();
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
 * pbdrv_bluetooth_stdin_get_free() to ensure enough free space.
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
 * Transmits data over any connected transport that is subscribed to Pybricks
 * protocol events.
 *
 * This may perform partial writes. Callers should check the number of bytes
 * actually written and call again with the remaining data until all data is
 * written.
 *
 * @param data  [in]        The data to transmit.
 * @param size  [inout]     The size of the data to transmit. Upon success, this
 *                          contains the number of bytes actually processed.
 * @return                  ::PBIO_ERROR_INVALID_OP if there is no active transport,
 *                          ::PBIO_ERROR_AGAIN if no @p data could be queued,
 *                          ::PBIO_SUCCESS if at least some data was queued.
 */
pbio_error_t pbsys_host_stdout_write(const uint8_t *data, uint32_t *size) {
    #if BLE_ONLY
    return pbdrv_bluetooth_tx(data, size);
    #elif USB_ONLY
    return pbdrv_usb_stdout_tx(data, size);
    #elif BLE_AND_USB

    uint32_t bt_avail = pbdrv_bluetooth_tx_available();
    uint32_t usb_avail = pbdrv_usb_stdout_tx_available();
    uint32_t available = bt_avail < usb_avail ? bt_avail : usb_avail;

    // If all tx_available() calls returned UINT32_MAX, then there is one listening.
    if (available == UINT32_MAX) {
        return PBIO_ERROR_INVALID_OP;
    }
    // If one or more tx_available() calls returned 0, then we need to wait.
    if (available == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // Limit size to smallest available space from all transports so that we
    // don't do partial writes to one transport and not the other.
    if (*size > available) {
        *size = available;
    }

    // Unless something became disconnected in an interrupt handler, these
    // functions should always succeed since we already checked tx_available().
    // And if both somehow got disconnected at the same time, it is not a big deal
    // if we return PBIO_SUCCESS without actually sending anything.
    (void)pbdrv_bluetooth_tx(data, size);
    (void)pbdrv_usb_stdout_tx(data, size);

    return PBIO_SUCCESS;

    #else
    // stdout goes to /dev/null
    return PBIO_SUCCESS;
    #endif
}

/**
 * Checks if all data has been transmitted.
 *
 * This is used to implement, e.g. a flush() function that blocks until all
 * data has been sent.
 *
 * @return              true if all data has been transmitted or no one is
 *                      listening, false if there is still data queued to be sent.
 */
bool pbsys_host_tx_is_idle(void) {
    // The USB part is a bit of a hack since it depends on the USB driver not
    // buffering more than one packet at a time to actually be accurate.
    #if BLE_ONLY
    return pbdrv_bluetooth_tx_is_idle();
    #elif USB_ONLY
    return pbdrv_usb_stdout_tx_is_idle();
    #elif BLE_AND_USB
    return pbdrv_bluetooth_tx_is_idle() && pbdrv_usb_stdout_tx_is_idle();
    #else
    return true;
    #endif
}

/**
 * Transmits data over any connected transport that is subscribed to Pybricks
 * protocol events, and awaits until it is written.
 *
 * @param event_type  [in]  Event type.
 * @param data        [in]  The data to transmit.
 * @param size        [in]  The size of the data to transmit.
 *                          contains the number of bytes actually processed.
 * @return                  ::PBIO_ERROR_AGAIN while the operation is in progress.
 *                          ::PBIO_ERROR_INVALID_ARG if invalid event type or size too big.
 *                          ::PBIO_ERROR_INVALID_OP if there is no connection to send it to.
 *                          ::PBIO_ERROR_BUSY if another transfer is already queued or in progress.
 *                          ::PBIO_SUCCESS on completion.
 */
pbio_error_t pbsys_host_send_event(pbio_os_state_t *state, pbio_pybricks_event_t event_type, const uint8_t *data, size_t size) {
    #if BLE_ONLY
    return pbdrv_bluetooth_send_event_notification(state, event_type, data, size);
    #elif USB_ONLY
    return pbdrv_usb_send_event_notification(state, event_type, data, size);
    #endif

    static pbio_os_state_t ble_state;
    static pbio_os_state_t usb_state;

    static pbio_error_t ble_err;
    static pbio_error_t usb_err;

    PBIO_OS_ASYNC_BEGIN(state);

    PBIO_OS_AWAIT_GATHER(state,
        &ble_err, &ble_state, pbdrv_bluetooth_send_event_notification(&ble_state, event_type, data, size),
        &usb_err, &usb_state, pbdrv_usb_send_event_notification(&usb_state, event_type, data, size)
        );

    // If any output is successful, the whole will be considered successful.
    if (ble_err == PBIO_SUCCESS || usb_err == PBIO_SUCCESS) {
        return PBIO_SUCCESS;
    }

    // Both have failed. In most cases, this means being fully disconnected so
    // it does not matter which one we return. Otherwise return the ble error.
    PBIO_OS_ASYNC_END(ble_err != PBIO_SUCCESS ? ble_err : usb_err);
}

#endif // PBSYS_CONFIG_HOST
