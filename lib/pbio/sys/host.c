// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_HOST

#include <string.h>

#include <lwrb/lwrb.h>

#include <pbdrv/bluetooth.h>
#include <pbdrv/config.h>
#include <pbio/int_math.h>
#include <pbio/protocol.h>
#include <pbio/serial.h>
#include <pbio/version.h>

#include <pbsys/command.h>
#include <pbsys/host.h>
#include <pbsys/hmi.h>
#include <pbsys/storage.h>

#include <pbsys/telemetry.h>

static pbsys_host_stdin_event_callback_t pbsys_host_stdin_event_callback;
static lwrb_t pbsys_host_stdin_ring_buf;
static lwrb_t pbsys_host_stdout_ring_buf;
static bool pbsys_host_event_stdout_busy;

// Hub name goes in a special section so that it can be modified when flashing
// firmware. The attribute syntax is only valid on ELF targets.
#ifdef __ELF__
__attribute__((section(".name")))
#endif
static char pbsys_host_hub_name[PBSYS_HOST_HUB_NAME_SIZE] = "Pybricks";

/**
 * Gets the hub name, reported to hosts where space is limited (BLE
 * advertisement) or where the bare name is expected (GATT device name and
 * Pybricks Profile reads, which hosts round-trip when renaming the hub).
 *
 * @return  The hub name as a null-terminated string.
 */
const char *pbsys_host_get_hub_name(void) {
    return pbsys_host_hub_name;
}

/**
 * Gets the hub name decorated with the hub type, e.g.
 * "myhub (SPIKE Prime)".
 *
 * Unlike the bare name from pbsys_host_get_hub_name(), this is used on
 * transports where the hub appears in host OS device pickers alongside
 * other devices, so it needs to be self-describing.
 *
 * @param [in] transport    The transport whose name to include.
 * @return                  The display name as a null-terminated string.
 */
const char *pbsys_host_get_hub_display_name(void) {
    static char display_name[PBSYS_HOST_HUB_DISPLAY_NAME_SIZE];
    strcpy(display_name, pbsys_host_hub_name);
    strcat(display_name, " (" PBSYS_CONFIG_HUB_TYPE_STR ")");
    return display_name;
}

void pbsys_host_init(void) {
    // Buffer needs 1 byte of headroom, but command drops 1 command.
    static uint8_t stdin_buf[PBSYS_CONFIG_HOST_EVENT_OUT_SIZE + 1 - 1];
    lwrb_init(&pbsys_host_stdin_ring_buf, stdin_buf, PBIO_ARRAY_SIZE(stdin_buf));

    // There is no technical constraint on this one. Can be reduced to save on
    // bss size. Heuristically, this gives users about two outgoing frames
    // worth of buffering before print pauses the user program.
    static uint8_t stdout_buf[PBSYS_CONFIG_HOST_EVENT_OUT_SIZE * 2 + 1];
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
 * Tests if the given transport has an active connection to a Pybricks app.
 *
 * @param [in] transport    The transport to test.
 * @return                  @c true if the connection is active, else @c false.
 */
static bool pbsys_host_transport_is_connected(pbsys_host_transport_type_t transport) {
    switch (transport) {
        case PBSYS_HOST_TRANSPORT_TYPE_BLUETOOTH:
            return pbdrv_bluetooth_host_is_connected();
        case PBSYS_HOST_TRANSPORT_TYPE_USB:
        case PBSYS_HOST_TRANSPORT_TYPE_RFCOMM:
            return pbio_serial_connection_is_active(transport);
        default:
            return false;
    }
}

/**
 * Tests if the hub is connected to the host on any transport.
 *
 * Connected implies an active connection to a Pybricks app, not just
 * physically plugged in.
 *
 * @return              @c true if connection is active, else @c false.
 */
bool pbsys_host_is_connected(void) {
    for (pbsys_host_transport_type_t transport = 0; transport < PBSYS_HOST_TRANSPORT_TYPE_COUNT; transport++) {
        if (pbsys_host_transport_is_connected(transport)) {
            return true;
        }
    }
    return false;
}

/**
 * Fills @p buf with the Pybricks hub capabilities characteristic value.
 *
 * Single source of truth for the feature flags, maximum program size and
 * number of program slots, shared by USB and all BLE drivers.
 *
 * @param [out] buf         Buffer of at least
 *                          ::PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE bytes.
 * @param [in]  transport   The transport the host is reading this value on,
 *                          which determines the reported maximum write size.
 */
void pbsys_host_get_hub_capabilities(uint8_t *buf, pbsys_host_transport_type_t transport) {

    // Serial transports are configured to allow the configured host event
    // size, while BLE is limited by the negotiated MTU.
    uint32_t max_receive_size = transport == PBSYS_HOST_TRANSPORT_TYPE_BLUETOOTH ?
        pbdrv_bluetooth_get_max_message_size():
        PBSYS_CONFIG_HOST_EVENT_OUT_SIZE - 1;

    pbio_pybricks_hub_capabilities(buf, max_receive_size,
        PBSYS_CONFIG_APP_FEATURE_FLAGS,
        pbsys_storage_get_maximum_program_size(),
        PBSYS_CONFIG_HMI_NUM_SLOTS);
}

static uint32_t pbsys_host_copy_str(uint8_t *buf, uint32_t buf_size, const char *str) {
    uint32_t size = strlen(str);
    if (size > buf_size) {
        size = buf_size;
    }
    memcpy(buf, str, size);
    return size;
}

/**
 * Provides characteristic values when a host requests a read by identifier,
 * used on transports without a GATT server of their own (currently serial).
 *
 * BLE drivers serve the same values through their own GATT servers, sourced
 * the from the same values.
 *
 * @param [in]  service             Which service to read.
 * @param [in]  char_id             16-bit characteristic id within @p service.
 * @param [in]  transport           The transport the request came in on.
 * @param [out] buf                 Buffer to receive the value.
 * @param [in]  buf_size            Size of @p buf. String values are truncated to fit.
 * @return                          Number of bytes written, or 0 if the
 *                                  characteristic is unknown or does not fit.
 */
uint32_t pbsys_host_read_characteristic(uint8_t service, uint16_t char_id, pbsys_host_transport_type_t transport, uint8_t *buf, uint32_t buf_size) {
    switch (service) {
        case PBIO_PYBRICKS_READ_SERVICE_GATT:
            switch (char_id) {
                case PBIO_GATT_DEVICE_NAME_CHAR_UUID:
                    return pbsys_host_copy_str(buf, buf_size, pbsys_host_get_hub_name());
                case PBIO_GATT_FIRMWARE_VERSION_CHAR_UUID:
                    return pbsys_host_copy_str(buf, buf_size, PBIO_VERSION_STR);
                case PBIO_GATT_SOFTWARE_VERSION_CHAR_UUID:
                    return pbsys_host_copy_str(buf, buf_size, PBIO_PROTOCOL_VERSION_STR);
                case PBIO_GATT_PNP_ID_CHAR_UUID:
                    if (buf_size < PBIO_PYBRICKS_PNP_ID_SIZE) {
                        return 0;
                    }
                    pbio_pybricks_pnp_id(buf, PBDRV_CONFIG_HUB_KIND, PBDRV_CONFIG_HUB_VARIANT);
                    return PBIO_PYBRICKS_PNP_ID_SIZE;
                default:
                    return 0;
            }
        case PBIO_PYBRICKS_READ_SERVICE_PYBRICKS:
            switch (char_id) {
                case 0x0003: // Hub capabilities
                    if (buf_size < PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE) {
                        return 0;
                    }
                    pbsys_host_get_hub_capabilities(buf, transport);
                    return PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE;
                default:
                    return 0;
            }
        default:
            return 0;
    }
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
    if (!pbsys_host_is_connected()) {
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
 * Per-transport size latch: how much of the current transmission each
 * transport still has to send. Zero when done (or not connected).
 */
static uint32_t pbsys_host_event_out_sizes[PBSYS_HOST_TRANSPORT_TYPE_COUNT];

/**
 * Marks the staged event of given @p size for transmission by all transports,
 * guarding the current transmission. Disconnected transports are cleared
 * again on their next pickup attempt.
 */
static void pbsys_host_event_out_set_size_all(uint32_t size) {
    for (pbsys_host_transport_type_t t = 0; t < PBSYS_HOST_TRANSPORT_TYPE_COUNT; t++) {
        pbsys_host_event_out_sizes[t] = size;
    }
}

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

    static uint8_t *current_buf;

    // Re-send status occasionally for if missed on flaky connection.
    static pbio_os_timer_t status_timer = {
        .duration = 500,
    };

    // Returns the relevant busy state for the requested transport.
    *len = &pbsys_host_event_out_sizes[transport];

    // Handle possible completion on ongoing transmission.
    if (pbsys_host_event_out_busy) {
        // Clear locks if disconnected and check if any transport still going.
        bool any_transport_busy = false;
        for (pbsys_host_transport_type_t t = 0; t < PBSYS_HOST_TRANSPORT_TYPE_COUNT; t++) {
            if (!pbsys_host_transport_is_connected(t)) {
                pbsys_host_event_out_sizes[t] = 0;
            }
            any_transport_busy = any_transport_busy || pbsys_host_event_out_sizes[t] != 0;
        }
        if (any_transport_busy) {
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
    }

    // Prepare a new transmission if any, prioritizing events by type, status first.

    // Prepare status.
    if (pbsys_host_status_data_pending || pbio_os_timer_is_expired(&status_timer)) {
        // When a status is pending, drain it here while we write it out,
        // so a new status can be set in the mean time without losing it.
        // The status already starts with the event type.
        //
        memcpy(&pbsys_host_event_out_buf[0], pbsys_host_status_data, PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
        pbsys_host_event_out_set_size_all(PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
        pbsys_host_status_data_pending = false;
        pbio_os_timer_reset(&status_timer);

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
        pbsys_host_event_out_set_size_all(drained_size + 1);

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
        pbsys_host_event_out_set_size_all(pbsys_host_app_data_size + 1);
        // Keep the pointer latched until transmitted; zero size marks pickup.
        pbsys_host_app_data_size = 0;

        // Initiatiate transfer.
        current_buf = *buf = pbsys_host_event_out_buf;
        pbsys_host_event_out_busy = true;
        return true;
    }

    // Telemetry, if pending, is sent from its own buffer without copying.
    uint32_t telemetry_size = pbsys_telemetry_get_data(pbsys_host_event_out_buf, pbsys_host_get_max_message_size());
    if (telemetry_size) {
        pbsys_host_event_out_set_size_all(telemetry_size);
        current_buf = *buf = pbsys_host_event_out_buf;
        pbsys_host_event_out_busy = true;
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
