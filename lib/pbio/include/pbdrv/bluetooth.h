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
 * Initiates a connection to a Bluetooth Classic HID device such as a gamepad.
 *
 * This is non-blocking. The driver handles the remaining steps, including
 * pairing if the device is in pairing mode. Poll
 * pbdrv_bluetooth_classic_hid_is_connected() for the result.
 *
 * @param [in] bdaddr  6-byte Bluetooth address of the device, as found with
 *                     an inquiry scan.
 * @return             ::PBIO_SUCCESS if the connection was initiated.
 *                     ::PBIO_ERROR_INVALID_OP if Bluetooth is not powered on.
 *                     ::PBIO_ERROR_BUSY if a connection is already in progress.
 *                     ::PBIO_ERROR_FAILED if the connection could not be started.
 */
pbio_error_t pbdrv_bluetooth_classic_hid_connect(const uint8_t *bdaddr);

/**
 * Tests whether a Bluetooth Classic HID device is connected.
 *
 * @return  True if connected.
 */
bool pbdrv_bluetooth_classic_hid_is_connected(void);

/**
 * Disconnects the Bluetooth Classic HID device or aborts an ongoing
 * connection attempt, if any.
 */
void pbdrv_bluetooth_classic_hid_disconnect(void);

#else // PBDRV_CONFIG_BLUETOOTH_CLASSIC

static inline pbio_error_t pbdrv_bluetooth_inquiry_start(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_inquiry_stop(void) {
}

static inline pbio_error_t pbdrv_bluetooth_inquiry_get_results(uint32_t *num, pbio_bluetooth_inquiry_result_t **results) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_classic_hid_connect(const uint8_t *bdaddr) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline bool pbdrv_bluetooth_classic_hid_is_connected(void) {
    return false;
}

static inline void pbdrv_bluetooth_classic_hid_disconnect(void) {
}

#endif // PBDRV_CONFIG_BLUETOOTH_CLASSIC

#endif // _INTERNAL_PBDRV_BLUETOOTH_H_
