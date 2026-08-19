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

/**
 * Context for an ongoing classic Bluetooth task.
 *
 * Not all fields are used by all functions. Only one function runs at a time.
 */
typedef struct {
    /**
     * The currently active function.
     */
    pbio_os_process_func_t func;
    /**
     * The most recent result of calling above function from main process.
     */
    pbio_error_t err;
    /**
     * Cancellation requested.
     */
    bool cancel;
    /**
     *  Watchdog for above operation so it can be cancelled on inactivity.
     */
    pbio_os_timer_t watchdog;
    /**
     * Inquiry scan results.
     */
    pbio_bluetooth_inquiry_result_t *inq_results;
    /**
     * Number of scan results found so far.
     */
    uint32_t *inq_count;
    /**
     * Maximum number of scan results to find.
     */
    uint32_t *inq_count_max;
    /**
     * Inquiry duration in units of 1.28 seconds.
     */
    uint8_t inq_duration;
} pbdrv_bluetooth_classic_task_context_t;

pbio_error_t pbdrv_bluetooth_inquiry_scan_func(pbio_os_state_t *state, void *context);

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

#endif // _INTERNAL_PBDRV_BLUETOOTH_H_
