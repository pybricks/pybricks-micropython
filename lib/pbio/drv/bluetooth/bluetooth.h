// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Internal common bluetooth functions.

#ifndef _INTERNAL_PBDRV_BLUETOOTH_H_
#define _INTERNAL_PBDRV_BLUETOOTH_H_

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH

#include <pbio/os.h>

#include <pbdrv/bluetooth.h>

#include <stdbool.h>
#include <stdint.h>

void pbdrv_bluetooth_init_hci(void);
void pbdrv_bluetooth_controller_reset_hard(void);
pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer);
pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer);

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context);

pbdrv_bluetooth_peripheral_t *pbdrv_bluetooth_peripheral_get_by_index(uint8_t index);
pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context);
pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context);

pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size);

extern pbdrv_bluetooth_receive_handler_t pbdrv_bluetooth_receive_handler;

extern uint8_t pbdrv_bluetooth_broadcast_data[PBDRV_BLUETOOTH_MAX_ADV_SIZE];
extern uint8_t pbdrv_bluetooth_broadcast_data_size;

typedef enum {
    PBDRV_BLUETOOTH_ADVERTISING_STATE_NONE,
    PBDRV_BLUETOOTH_ADVERTISING_STATE_ADVERTISING_PYBRICKS,
    PBDRV_BLUETOOTH_ADVERTISING_STATE_BROADCASTING,
} pbdrv_bluetooth_advertising_state_t;

extern pbdrv_bluetooth_advertising_state_t pbdrv_bluetooth_advertising_state;

extern bool pbdrv_bluetooth_is_observing;
extern pbdrv_bluetooth_start_observing_callback_t pbdrv_bluetooth_observe_callback;

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
    pbdrv_bluetooth_inquiry_result_t *inq_results;
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

// The following functions are defined in each Bluetooth implementation that
// supports RFCOMM sockets.

// Cancels all pending connection attempts. Called e.g. when the user interrupts
// execution to drop into the debug REPL.
void pbdrv_bluetooth_rfcomm_cancel_connection();

// Disconnects all active RFCOMM connections and cleans up all sockets. Called
// during user program termination.
void pbdrv_bluetooth_rfcomm_disconnect_all();

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _INTERNAL_PBDRV_BLUETOOTH_H_
