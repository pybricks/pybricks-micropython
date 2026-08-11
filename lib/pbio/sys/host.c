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
static lwrb_t pbsys_host_stdout_ring_buf;
static bool pbsys_host_event_stdout_busy;

void pbsys_host_init(void) {
    static uint8_t stdin_buf[PBSYS_CONFIG_HOST_STDIN_BUF_SIZE];
    lwrb_init(&pbsys_host_stdin_ring_buf, stdin_buf, PBIO_ARRAY_SIZE(stdin_buf));

    static uint8_t stdout_buf[PBSYS_CONFIG_HOST_STDOUT_BUF_SIZE];
    lwrb_init(&pbsys_host_stdout_ring_buf, stdout_buf, PBIO_ARRAY_SIZE(stdout_buf));

    pbdrv_bluetooth_set_receive_handler(pbsys_command);
    pbdrv_usb_set_receive_handler(pbsys_command);
}

/**
 * Buffer scheduled status.
 */
static uint8_t pbsys_host_status_data[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
static bool pbsys_host_status_data_pending;

/**
 * Schedules sending a status update to connected hosts.
 *
 * The data length is always ::PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE.
 */
void pbsys_host_schedule_status_update(const uint8_t *status_msg) {
    // Ignore if message identical to last.
    if (!memcmp(pbsys_host_status_data, status_msg, sizeof(pbsys_host_status_data))) {
        return;
    }

    // Schedule to send whenever transmission processes get round to it.
    memcpy(pbsys_host_status_data, status_msg, sizeof(pbsys_host_status_data));
    pbsys_host_status_data_pending = true;
    pbio_os_request_poll();
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
    // Fail if no one is listening.
    if (!pbdrv_bluetooth_host_is_connected() && !pbdrv_usb_connection_is_active()) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Wait if full.
    uint32_t free_size = lwrb_get_free(&pbsys_host_stdout_ring_buf);
    if (free_size == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // Limit size to available space.
    if (*size > free_size) {
        *size = free_size;
    }

    // Buffer data to send it more efficiently even if the caller is only
    // writing one byte at a time.
    if ((*size = lwrb_write(&pbsys_host_stdout_ring_buf, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // poke the process to start tx soon-ish. This way, we can accumulate
    // multiple messages before actually transmitting.
    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

void pbsys_host_debug_print(const char *data, size_t len) {

    if (!lwrb_is_ready(&pbsys_host_stdout_ring_buf)) {
        return;
    }

    // Buffer result with \r injected before \n.
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            lwrb_write(&pbsys_host_stdout_ring_buf, (const uint8_t *)"\r", 1);
        }
        lwrb_write(&pbsys_host_stdout_ring_buf, (const uint8_t *)&data[i], 1);
    }

    pbio_os_request_poll();
}


#if PBDRV_CONFIG_BLUETOOTH && (PBSYS_CONFIG_HOST_EVENT_OUT_SIZE > PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE - PBDRV_BLUETOOTH_ATT_HEADER_SIZE)
#error "Host event message must fit in one BLE packet".
#endif

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
    if (!pbsys_host_is_connected()) {
        return true;
    }

    return lwrb_get_full(&pbsys_host_stdout_ring_buf) == 0 && !pbsys_host_event_stdout_busy;
}

pbio_error_t pbsys_host_get_event_buf(pbsys_host_transport_type_t transport, uint8_t **buf, uint32_t **len) {

    static uint8_t pbsys_host_event_out_buf[PBSYS_CONFIG_HOST_EVENT_OUT_SIZE];
    static bool pbsys_host_event_out_busy;

    static uint32_t bluetooth_size;
    static uint32_t usb_size;
    static uint8_t *current_buf;

    // Returns the relevant busy state for the requested transport.
    *len = transport == PBSYS_HOST_TRANSPORT_TYPE_BLUETOOTH ? &bluetooth_size : &usb_size;

    if (pbsys_host_event_out_busy) {
        // Clear locks if disconnected.
        if (!pbdrv_bluetooth_host_is_connected()) {
            bluetooth_size = 0;
        }
        if (!pbdrv_usb_connection_is_active()) {
            usb_size = 0;
        }
        if (bluetooth_size || usb_size) {
            // At least one transport is still going, so keep referencing
            // current data, no matter which transport initiated first.
            *buf = current_buf;
            return PBIO_ERROR_AGAIN;
        }
        // Last transmission is complete, so we can initiate another.
        pbsys_host_event_out_busy = false;
        pbsys_host_event_stdout_busy = false;
    }

    // Prepare status.
    if (pbsys_host_status_data_pending) {
        // When a status is pending, drain it here while we write it out,
        // so a new status can be set in the mean time without losing it.
        // The status already starts with the event type.
        //
        memcpy(&pbsys_host_event_out_buf[0], pbsys_host_status_data, PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
        usb_size = bluetooth_size = PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE;
        pbsys_host_status_data_pending = false;
        // This is the buffer to send; ready to get started.
        *buf = pbsys_host_event_out_buf;
        pbsys_host_event_out_busy = true;
        return PBIO_ERROR_AGAIN;
    }

    // Prepare stdout, drain into chunk of maximum send size.
    if (lwrb_get_full(&pbsys_host_stdout_ring_buf) != 0) {
        // Message always starts with event byte.
        pbsys_host_event_out_buf[0] = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;

        // Drain ring buffer to send buffer as much as we can. Limit is the
        // runtime MTU. USB sizes are protected by constant build flags.
        uint32_t mtu = pbdrv_bluetooth_get_max_message_size();
        uint32_t size = 1 + lwrb_read(&pbsys_host_stdout_ring_buf, &pbsys_host_event_out_buf[1], mtu - 1);

        // All transports are marked to send the same size, guarding current transmission.
        usb_size = size;
        bluetooth_size = size;

        // This is the buffer to send; ready to get started.
        *buf = pbsys_host_event_out_buf;
        pbsys_host_event_out_busy = true;
        return PBIO_ERROR_AGAIN;
    }

    // TODO: Queue generic numbered events from pbsys_host_send_event.

    // Nothing to do.
    return PBIO_SUCCESS;
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
    // Todo: copy data to local host buffer or use without copying.
    return PBIO_ERROR_NOT_IMPLEMENTED;
}
#endif // PBSYS_CONFIG_HOST
