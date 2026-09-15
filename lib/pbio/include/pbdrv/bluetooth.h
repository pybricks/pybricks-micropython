// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Internal common bluetooth functions.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_H_
#define _INTERNAL_PBDRV_BLUETOOTH_H_

#include <pbdrv/config.h>

#include <pbio/os.h>

#include <pbio/bluetooth.h>

#include <stdbool.h>
#include <stdint.h>

#if PBDRV_CONFIG_BLUETOOTH

void pbdrv_bluetooth_init(void);

/**
 * Gets the Bluetooth chip firmware version.
 *
 * This should not be called until after the Bluetooth chip is powered on and
 * initalized.
 *
 * @returns A string describing the version or an empty string if not supported.
 */
const char *pbdrv_bluetooth_get_fw_version(void);

/**
 * Tests if at least one central is connected to the Bluetooth chip and
 * subscribed to Pybricks events.
 *
 * @param [in]  connection  The type of connection of interest.
 * @return                  True if Pybricks host connected, otherwise false.
 */
bool pbdrv_bluetooth_host_is_connected(void);

/**
 * Tests if the Bluetooth controller is up and running.
 *
 * @return                  True if running,
 *                          otherwise false.
 */
bool pbdrv_bluetooth_hci_is_enabled(void);

/**
 * Gets the maximum Pybricks message size (the full notification value,
 * including the leading event type byte) that can be sent to the host.
 *
 * For BLE this is the negotiated ATT MTU minus the 3-byte notification header,
 * taken as the minimum over all connected hosts and capped at the platform's
 * maximum MTU. Callers must not send more than this many bytes in a single
 * notification. The actual user payload is one byte less (the event type byte).
 *
 * @return              The maximum message size in bytes.
 */
uint16_t pbdrv_bluetooth_get_max_message_size(void);

/**
 * Checks if the given peripheral is connected.
 *
 * @param [in]  peri    The peripheral to check.
 * @return              True if connected, false otherwise.
 */
bool pbdrv_bluetooth_peripheral_is_connected(pbio_bluetooth_peripheral_t *peri);

pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer);
pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer);
pbio_error_t pbdrv_bluetooth_disconnect_all(pbio_os_state_t *state);

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context);

pbio_bluetooth_peripheral_t *pbdrv_bluetooth_peripheral_get_by_index(uint8_t index);
pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context);

pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size);

void pbio_bluetooth_host_connection_changed(void);

pbio_pybricks_error_t pbio_bluetooth_receive_handler(const uint8_t *data, uint32_t size);

extern uint8_t pbdrv_bluetooth_broadcast_data[PBIO_BLUETOOTH_MAX_ADV_SIZE];
extern uint8_t pbdrv_bluetooth_broadcast_data_size;

typedef enum {
    PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE,
    PBDRV_BLUETOOTH_ADVERTISING_STATE_ADVERTISING_PYBRICKS,
    PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING,
} pbdrv_bluetooth_advertising_state_t;

extern pbdrv_bluetooth_advertising_state_t pbdrv_bluetooth_advertising_state;

extern bool pbdrv_bluetooth_is_observing;
extern pbio_bluetooth_start_observing_callback_t pbdrv_bluetooth_observe_callback;

pbio_error_t pbdrv_bluetooth_process_thread(pbio_os_state_t *state, void *context);

#else // PBDRV_CONFIG_BLUETOOTH

static inline void pbdrv_bluetooth_init(void) {
}

static inline const char *pbdrv_bluetooth_get_fw_version(void) {
    return NULL;
}

static inline bool pbdrv_bluetooth_host_is_connected(void) {
    return false;
}

static inline bool pbdrv_bluetooth_hci_is_enabled(void) {
    return false;
}

static inline uint16_t pbdrv_bluetooth_get_max_message_size(void) {
    return UINT16_MAX;
}

static inline bool pbdrv_bluetooth_peripheral_is_connected(pbio_bluetooth_peripheral_t *peri) {
    return false;
}

#endif // PBDRV_CONFIG_BLUETOOTH

/**
 * Maximum number of inquiry scan results stored by the driver.
 */
#define PBDRV_BLUETOOTH_INQUIRY_NUM_RESULTS (20)

/**
 * Size of the buffer holding the most recent Bluetooth Classic HID input
 * report. Enough for the 78-byte DualSense extended report.
 */
#define PBDRV_BLUETOOTH_HID_MAX_REPORT_SIZE (80)

/**
 * Number of distinct Bluetooth Classic HID input report IDs stored at once.
 */
#define PBDRV_BLUETOOTH_HID_NUM_REPORTS (4)

#if PBDRV_CONFIG_BLUETOOTH_CLASSIC

/**
 * Starts an inquiry scan for Bluetooth Classic devices.
 *
 * Resets previously found results. The scan runs for a fixed duration and
 * then stops on its own. Callers can start a new scan to keep scanning.
 *
 * @return  ::PBIO_SUCCESS on success.
 *          ::PBIO_ERROR_INVALID_OP if Bluetooth is not powered on.
 *          ::PBIO_ERROR_BUSY if a scan is already in progress.
 *          ::PBIO_ERROR_FAILED if the scan could not be started.
 */
pbio_error_t pbdrv_bluetooth_inquiry_start(void);

/**
 * Stops an ongoing inquiry scan, if any.
 */
void pbdrv_bluetooth_inquiry_stop(void);

/**
 * Gets the results of the ongoing inquiry scan.
 *
 * @param [out] num      Number of results found so far.
 * @param [out] results  The results.
 * @return               ::PBIO_SUCCESS on success.
 *                       ::PBIO_ERROR_INVALID_OP if no scan is in progress.
 */
pbio_error_t pbdrv_bluetooth_inquiry_get_results(uint32_t *num, pbio_bluetooth_inquiry_result_t **results);

/**
 * Starts pairing with a Bluetooth Classic HID device such as a gamepad.
 *
 * This is non-blocking. The driver registers a provisional bonding record
 * for the device and handles the remaining steps, storing the negotiated
 * link key into the record on success or forgetting it again on failure.
 * Poll pbdrv_bluetooth_classic_hid_pair_status() for the result.
 *
 * @param [in] bdaddr  6-byte Bluetooth address of the device, as found with
 *                     an inquiry scan.
 * @param [in] name    Device name for the bonding record.
 * @return             ::PBIO_SUCCESS if pairing was initiated.
 *                     ::PBIO_ERROR_INVALID_OP if Bluetooth is not powered on.
 *                     ::PBIO_ERROR_BUSY if connecting or already connected.
 *                     ::PBIO_ERROR_FAILED if pairing could not be started.
 */
pbio_error_t pbdrv_bluetooth_classic_hid_pair(const uint8_t *bdaddr, const char *name);

/**
 * Gets the status of pairing started with pbdrv_bluetooth_classic_hid_pair().
 *
 * @return  ::PBIO_ERROR_AGAIN while pairing is in progress, ::PBIO_SUCCESS
 *          if the last attempt succeeded, ::PBIO_ERROR_TIMEDOUT or
 *          ::PBIO_ERROR_CANCELED if it timed out or was cancelled.
 */
pbio_error_t pbdrv_bluetooth_classic_hid_pair_status(void);

/**
 * Cancels an ongoing pairing attempt, if any, forgetting the provisional
 * bonding record.
 */
void pbdrv_bluetooth_classic_hid_pair_cancel(void);

/**
 * Tests whether a Bluetooth Classic HID device is connected.
 *
 * @return  True if connected.
 */
bool pbdrv_bluetooth_classic_hid_is_connected(void);

/**
 * Gets the most recent input report with the given ID from the connected
 * Bluetooth Classic HID device.
 *
 * HID devices push input reports on their own; there is nothing to subscribe
 * to. A report ID becomes available only after the device has sent it at
 * least once since connecting.
 *
 * @param [in]  report_id  ID of the requested report, or 0 if the device
 *                         declares no report IDs.
 * @param [out] data       Buffer to copy the report into, starting with the
 *                         report ID if the device has any.
 * @param [in]  size       Size of @p data.
 * @return                 Number of bytes copied, or 0 if nothing is
 *                         connected or this report has not been seen yet.
 */
uint32_t pbdrv_bluetooth_classic_hid_get_report(uint8_t report_id, uint8_t *data, uint32_t size);

/**
 * Gets the ID of the input report stored in the given slot, to discover what
 * the connected device sends.
 *
 * @param [in]  index      Slot index, less than ::PBDRV_BLUETOOTH_HID_NUM_REPORTS.
 * @param [out] report_id  The report ID, if the slot is in use.
 * @return                 True if the slot is in use.
 */
bool pbdrv_bluetooth_classic_hid_get_report_id(uint32_t index, uint8_t *report_id);

/**
 * Gets the name of the connected Bluetooth Classic HID device.
 *
 * @return  The name, or NULL if nothing is connected or the device is not in
 *          the bonding store (it can connect without being registered).
 */
const char *pbdrv_bluetooth_classic_hid_get_connected_name(void);

/**
 * Disconnects the Bluetooth Classic HID device or aborts an ongoing
 * connection attempt, if any.
 */
void pbdrv_bluetooth_classic_hid_disconnect(void);

/**
 * Starts pairing with a host computer (PC).
 *
 * Uses dedicated bonding: the hub connects only to establish the bond and
 * drops the link when done. The bonded host then initiates the actual
 * serial (RFCOMM) connection itself, e.g. from a web browser via the serial
 * port the OS exposes for the hub.
 *
 * This is non-blocking. Poll pbdrv_bluetooth_classic_host_pair_status() for
 * the result.
 *
 * @param [in] bdaddr  6-byte Bluetooth address of the host, as found with
 *                     an inquiry scan.
 * @param [in] name    Host name for the bonding record.
 * @return             ::PBIO_SUCCESS if pairing was initiated.
 *                     ::PBIO_ERROR_INVALID_OP if Bluetooth is not powered on.
 *                     ::PBIO_ERROR_BUSY if pairing or already connected.
 *                     ::PBIO_ERROR_FAILED if pairing could not be started.
 */
pbio_error_t pbdrv_bluetooth_classic_host_pair(const uint8_t *bdaddr, const char *name);

/**
 * Gets the status of pairing started with pbdrv_bluetooth_classic_host_pair().
 *
 * @return  ::PBIO_ERROR_AGAIN while pairing is in progress, ::PBIO_SUCCESS
 *          if the last attempt succeeded, ::PBIO_ERROR_TIMEDOUT or
 *          ::PBIO_ERROR_CANCELED if it timed out or was cancelled.
 */
pbio_error_t pbdrv_bluetooth_classic_host_pair_status(void);

/**
 * Gets the numeric comparison passkey of the ongoing host pairing, once
 * available, so it can be shown for the user to verify against the
 * confirmation prompt on the host.
 *
 * @param [out] passkey  The 6-digit passkey.
 * @return               True if a passkey is currently available.
 */
bool pbdrv_bluetooth_classic_host_pair_passkey(uint32_t *passkey);

/**
 * Cancels an ongoing host pairing attempt, if any, forgetting the
 * provisional bonding record.
 */
void pbdrv_bluetooth_classic_host_pair_cancel(void);

/**
 * Tests whether a host computer is connected over RFCOMM.
 *
 * @return  True if connected.
 */
bool pbdrv_bluetooth_classic_host_is_connected(void);

/**
 * Gets the name of the connected host computer.
 *
 * @return  The name, or NULL if nothing is connected or the host is not in
 *          the bonding store (it can connect without being registered).
 */
const char *pbdrv_bluetooth_classic_host_get_connected_name(void);

/**
 * Disconnects the host computer RFCOMM connection, if any.
 */
void pbdrv_bluetooth_classic_host_disconnect(void);

/**
 * Reads bytes received on the host computer RFCOMM connection.
 *
 * The host to hub direction is a raw byte stream (message framing is handled
 * by the pbio serial process), so this returns an arbitrary slice of that
 * stream. Call repeatedly until it returns 0 to drain.
 *
 * @param [out] data    Buffer to copy the bytes to.
 * @param [in]  size    Maximum number of bytes to copy.
 * @return              Number of bytes copied. Zero if none are available.
 */
uint32_t pbdrv_bluetooth_classic_host_rx_read(uint8_t *data, uint32_t size);

/**
 * Sends a message on the host computer RFCOMM connection, chunked to the
 * negotiated frame size as needed. The caller ensures only one message is
 * in flight at a time and that @p data stays valid until completion.
 *
 * @param [in] state    Protothread state.
 * @param [in] data     Data to send.
 * @param [in] size     Data size.
 * @return              ::PBIO_SUCCESS when the message has been sent.
 *                      ::PBIO_ERROR_AGAIN while sending is in progress.
 *                      ::PBIO_ERROR_INVALID_OP if there is no connection or
 *                      it was lost while sending.
 */
pbio_error_t pbdrv_bluetooth_classic_host_tx_message(pbio_os_state_t *state, const uint8_t *data, uint32_t size);

#else // PBDRV_CONFIG_BLUETOOTH_CLASSIC

static inline pbio_error_t pbdrv_bluetooth_inquiry_start(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_inquiry_stop(void) {
}

static inline pbio_error_t pbdrv_bluetooth_inquiry_get_results(uint32_t *num, pbio_bluetooth_inquiry_result_t **results) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_classic_hid_pair(const uint8_t *bdaddr, const char *name) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_classic_hid_pair_status(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_classic_hid_pair_cancel(void) {
}

static inline bool pbdrv_bluetooth_classic_hid_is_connected(void) {
    return false;
}

static inline uint32_t pbdrv_bluetooth_classic_hid_get_report(uint8_t report_id, uint8_t *data, uint32_t size) {
    return 0;
}

static inline bool pbdrv_bluetooth_classic_hid_get_report_id(uint32_t index, uint8_t *report_id) {
    return false;
}

static inline const char *pbdrv_bluetooth_classic_hid_get_connected_name(void) {
    return NULL;
}

static inline void pbdrv_bluetooth_classic_hid_disconnect(void) {
}

static inline pbio_error_t pbdrv_bluetooth_classic_host_pair(const uint8_t *bdaddr, const char *name) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_classic_host_pair_status(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline bool pbdrv_bluetooth_classic_host_pair_passkey(uint32_t *passkey) {
    return false;
}

static inline void pbdrv_bluetooth_classic_host_pair_cancel(void) {
}

static inline bool pbdrv_bluetooth_classic_host_is_connected(void) {
    return false;
}

static inline const char *pbdrv_bluetooth_classic_host_get_connected_name(void) {
    return NULL;
}

static inline void pbdrv_bluetooth_classic_host_disconnect(void) {
}

static inline uint32_t pbdrv_bluetooth_classic_host_rx_read(uint8_t *data, uint32_t size) {
    return 0;
}

static inline pbio_error_t pbdrv_bluetooth_classic_host_tx_message(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BLUETOOTH_CLASSIC

#endif // _INTERNAL_PBDRV_BLUETOOTH_H_
