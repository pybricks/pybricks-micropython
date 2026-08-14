// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_HOST

#include <lwrb/lwrb.h>

#include <pbio/bluetooth.h>
#include <pbio/int_math.h>
#include <pbio/usb.h>

#include <pbsys/command.h>
#include <pbsys/host.h>
#include <pbsys/hmi.h>

#include "telemetry.h"

static pbsys_host_stdin_event_callback_t pbsys_host_stdin_event_callback;
static lwrb_t pbsys_host_stdin_ring_buf;
static lwrb_t pbsys_host_stdout_ring_buf;
static bool pbsys_host_event_stdout_busy;

void pbsys_host_init(void) {
    static uint8_t stdin_buf[PBSYS_CONFIG_HOST_STDIN_BUF_SIZE];
    lwrb_init(&pbsys_host_stdin_ring_buf, stdin_buf, PBIO_ARRAY_SIZE(stdin_buf));

    static uint8_t stdout_buf[PBSYS_CONFIG_HOST_STDOUT_BUF_SIZE];
    lwrb_init(&pbsys_host_stdout_ring_buf, stdout_buf, PBIO_ARRAY_SIZE(stdout_buf));
}

void pbsys_host_connection_changed(void) {
    pbsys_hmi_connection_changed_handler();
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
           pbio_usb_connection_is_active();
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
 * Gets the maximum message size that can be sent to the host on all active
 * connections. Accounts for event byte, so size is the payload.
 */
static uint32_t pbsys_host_get_max_message_size(void) {
    // USB limit is configured to allow configured host event size, so poses
    // no additional runtime limit.
    return pbio_int_math_min(pbdrv_bluetooth_get_max_message_size(), PBSYS_CONFIG_HOST_EVENT_OUT_SIZE) - 1;
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
    if (!pbdrv_bluetooth_host_is_connected() && !pbio_usb_connection_is_active()) {
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

/**
 * Shared buffer for one outgoing event message at a time, and whether a
 * transmission from it (or a telemetry buffer) is currently in progress.
 */
static uint8_t pbsys_host_event_out_buf[PBSYS_CONFIG_HOST_EVENT_OUT_SIZE];
static bool pbsys_host_event_out_busy;

/**
 * App data message staged for transmission (NULL if none). The data is owned
 * by the sender, which must keep it valid until it is copied out at
 * transmission time, or call pbsys_host_app_data_clear_pending() when it
 * can't, such as when it is about to be garbage collected. A zero size with
 * the pointer still set means it was copied and is now being transmitted.
 */
static const uint8_t *pbsys_host_app_data;
static size_t pbsys_host_app_data_size;

/**
 * Whether a telemetry transmission is in progress.
 */
static bool pbsys_host_event_telemetry_busy;

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

bool pbsys_host_get_event_buf(pbsys_host_transport_type_t transport, uint8_t **buf, uint32_t **len) {

    static uint32_t bluetooth_size;
    static uint32_t usb_size;
    static uint8_t *current_buf;

    // Returns the relevant busy state for the requested transport.
    *len = transport == PBSYS_HOST_TRANSPORT_TYPE_BLUETOOTH ? &bluetooth_size : &usb_size;

    // Handle possible completion on ongoing transmission.
    if (pbsys_host_event_out_busy) {
        // Clear locks if disconnected.
        if (!pbdrv_bluetooth_host_is_connected()) {
            bluetooth_size = 0;
        }
        if (!pbio_usb_connection_is_active()) {
            usb_size = 0;
        }
        if (bluetooth_size || usb_size) {
            // At least one transport is still going, so keep referencing
            // current data, no matter which transport initiated first. Only
            // resume the caller if it still has data itself, else it would
            // retransmit while waiting for the other transport to finish.
            *buf = current_buf;
            return **len != 0;
        }
        // Last transmission is complete, so we can initiate another.
        pbsys_host_event_out_busy = false;
        pbsys_host_event_stdout_busy = false;
        if (pbsys_host_app_data && !pbsys_host_app_data_size) {
            // App data was in flight; mark it fully transmitted.
            pbsys_host_app_data = NULL;
        }
        if (pbsys_host_event_telemetry_busy) {
            pbsys_host_event_telemetry_busy = false;
            pbsys_telemetry_data_sent();
        }
    }

    // Prepare a new transmission if any, prioritizing events by type, status first.

    // Prepare status.
    if (pbsys_host_status_data_pending) {
        // When a status is pending, drain it here while we write it out,
        // so a new status can be set in the mean time without losing it.
        // The status already starts with the event type.
        //
        memcpy(&pbsys_host_event_out_buf[0], pbsys_host_status_data, PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
        usb_size = bluetooth_size = PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE;
        pbsys_host_status_data_pending = false;

        // Initiatiate transfer.
        current_buf = *buf = pbsys_host_event_out_buf;
        pbsys_host_event_out_busy = true;
        return true;
    }

    // Prepare stdout, drain into chunk of maximum send size.
    if (lwrb_is_ready(&pbsys_host_stdout_ring_buf) && lwrb_get_full(&pbsys_host_stdout_ring_buf) != 0) {
        // Message always starts with event byte.
        pbsys_host_event_out_buf[0] = PBIO_PYBRICKS_EVENT_WRITE_STDOUT;

        // Drain ring buffer to send buffer as much as we can. Limit is the
        // runtime MTU minus one event byte.
        uint32_t drained_size = lwrb_read(&pbsys_host_stdout_ring_buf, &pbsys_host_event_out_buf[1], pbsys_host_get_max_message_size());

        // All transports are marked to send the same size, guarding current transmission.
        usb_size = bluetooth_size = drained_size + 1;

        // Initiatiate transfer, marking stdout busy.
        current_buf = *buf = pbsys_host_event_out_buf;
        pbsys_host_event_out_busy = true;
        pbsys_host_event_stdout_busy = true;
        return true;
    }

    // App data, if pending. Copied only now. Data is valid unless cleared
    // by the sender before it is picked up.
    if (pbsys_host_app_data_size) {
        pbsys_host_event_out_buf[0] = PBIO_PYBRICKS_EVENT_WRITE_APP_DATA;
        memcpy(&pbsys_host_event_out_buf[1], pbsys_host_app_data, pbsys_host_app_data_size);
        usb_size = bluetooth_size = pbsys_host_app_data_size + 1;
        // Keep the pointer latched until transmitted; zero size marks pickup.
        pbsys_host_app_data_size = 0;

        // Initiatiate transfer.
        current_buf = *buf = pbsys_host_event_out_buf;
        pbsys_host_event_out_busy = true;
        return true;
    }

    // Telemetry, if pending, is sent from its own buffer without copying.
    uint32_t telemetry_size;
    uint8_t *telemetry_data = pbsys_telemetry_get_data(&telemetry_size);
    if (telemetry_size) {
        usb_size = bluetooth_size = telemetry_size;
        current_buf = *buf = telemetry_data;
        pbsys_host_event_out_busy = true;
        pbsys_host_event_telemetry_busy = true;
        return true;
    }

    // Nothing to do.
    return false;
}

/**
 * Clears a pending app data message without sending it.
 *
 * The sender must call this when the pending data is about to become invalid,
 * e.g. from a garbage collection finalizer.
 */
void pbsys_host_app_data_clear_pending(void) {
    pbsys_host_app_data = NULL;
    pbsys_host_app_data_size = 0;
}

/**
 * Sends an app data event message to connected hosts, awaiting until it is
 * transmitted.
 *
 * This stages the given data for transmission after pending status and stdout,
 * where it is copied into the shared event buffer. Until then, the caller must
 * keep @p data valid, or call pbsys_host_app_data_clear_pending() when it
 * can't. Any previously staged message that was not yet picked up is replaced.
 *
 * @param state  [in]  Protothread state.
 * @param data   [in]  The data to transmit.
 * @param size   [in]  The size of the data to transmit.
 * @return             ::PBIO_ERROR_AGAIN while the operation is in progress.
 *                     ::PBIO_ERROR_INVALID_ARG if the size is too big.
 *                     ::PBIO_ERROR_INVALID_OP if there is no connection or it was lost.
 *                     ::PBIO_SUCCESS on completion.
 */
pbio_error_t pbsys_host_send_app_data(pbio_os_state_t *state, const uint8_t *data, size_t size) {

    PBIO_OS_ASYNC_BEGIN(state);

    if (size + 1 > PBSYS_CONFIG_HOST_EVENT_OUT_SIZE ||
        (pbdrv_bluetooth_host_is_connected() && size > pbsys_host_get_max_message_size())) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (!pbsys_host_is_connected()) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Nothing to send; would otherwise stage as already picked up.
    if (size == 0) {
        return PBIO_SUCCESS;
    }

    // Stage for pickup by pbsys_host_get_event_buf.
    pbsys_host_app_data = data;
    pbsys_host_app_data_size = size;
    pbio_os_request_poll();

    // Await pickup and transmission. The pointer is cleared when fully
    // transmitted, or replaced if a newer message superseded this one.
    PBIO_OS_AWAIT_UNTIL(state, !pbsys_host_is_connected() || pbsys_host_app_data != data);

    if (pbsys_host_app_data == data) {
        // Disconnected before or during transmission, so drop it.
        pbsys_host_app_data_clear_pending();
        return PBIO_ERROR_INVALID_OP;
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}
#endif // PBSYS_CONFIG_HOST
