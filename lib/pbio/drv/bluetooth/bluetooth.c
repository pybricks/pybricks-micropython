// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

// Common Bluetooth driver code

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH

#include <stdint.h>

#include <pbdrv/bluetooth.h>

#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#include "./bluetooth.h"

#define DEBUG 0

#if DEBUG
#include <pbdrv/../../drv/uart/uart_debug_first_port.h>
#define DEBUG_PRINT pbdrv_uart_debug_printf
#else
#define DEBUG_PRINT(...)
#endif

/**
 * Bluetooth power state. True means on (or on requested) False means off (or off requested).
 */
static bool power_on_requested;

/**
 * Bluetooth power enable busy state. True means that the operation is busy.
 *
 * Starts off in the reset state, so starts busy.
 */
static bool power_change_busy = true;

pbio_error_t pbdrv_bluetooth_power_on(pbio_os_state_t *state, bool on) {
    PBIO_OS_ASYNC_BEGIN(state);

    // Await ongoing state change if any.
    PBIO_OS_AWAIT_WHILE(state, power_change_busy);

    // Already in requested state.
    if (power_on_requested == on) {
        return PBIO_SUCCESS;
    }

    // Kick the Bluetooth process and await Bluetooth to become ready again.
    power_change_busy = true;
    power_on_requested = on;
    pbio_os_request_poll();
    PBIO_OS_AWAIT_WHILE(state, power_change_busy);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbdrv_bluetooth_receive_handler_t pbdrv_bluetooth_receive_handler;

void pbdrv_bluetooth_set_receive_handler(pbdrv_bluetooth_receive_handler_t handler) {
    pbdrv_bluetooth_receive_handler = handler;
}

//
// Functions related to sending value notifications (stdout, status, app data).
//

/**
 * Buffer scheduled status.
 */
static uint8_t status_data[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
static bool status_data_pending;

void pbdrv_bluetooth_schedule_status_update(const uint8_t *status_msg) {
    // Ignore if message identical to last.
    if (!memcmp(status_data, status_msg, sizeof(status_data))) {
        return;
    }

    // Schedule to send whenever the Bluetooth process gets round to it.
    memcpy(status_data, status_msg, sizeof(status_data));
    status_data_pending = true;
    pbio_os_request_poll();
}

/**
 * Buffer for scheduled stdout.
 */
static lwrb_t stdout_ring_buf;
static bool stdout_send_busy;

void pbdrv_bluetooth_init(void) {
    // enough for two packets, one currently being sent and one to be ready
    // as soon as the previous one completes + 1 byte for ring buf pointer
    static uint8_t stdout_buf[PBDRV_BLUETOOTH_MAX_CHAR_SIZE * 2 + 1];
    lwrb_init(&stdout_ring_buf, stdout_buf, PBIO_ARRAY_SIZE(stdout_buf));

    pbdrv_bluetooth_init_hci();
}

pbio_error_t pbdrv_bluetooth_tx(const uint8_t *data, uint32_t *size) {

    // make sure we have a Bluetooth connection
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Buffer data to send it more efficiently even if the caller is only
    // writing one byte at a time.
    if ((*size = lwrb_write(&stdout_ring_buf, data, *size)) == 0) {
        return PBIO_ERROR_AGAIN;
    }

    // poke the process to start tx soon-ish. This way, we can accumulate up to
    // PBDRV_BLUETOOTH_MAX_CHAR_SIZE bytes before actually transmitting
    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

uint32_t pbdrv_bluetooth_tx_available(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
        return UINT32_MAX;
    }

    return lwrb_get_free(&stdout_ring_buf);
}

bool pbdrv_bluetooth_tx_is_idle(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS)) {
        return true;
    }

    return !stdout_send_busy && lwrb_get_full(&stdout_ring_buf) == 0;
}

/**
 * Buffer for generic notification, but not buffered or schedules.
 */
static uint8_t user_notification_send_buf[PBDRV_BLUETOOTH_MAX_CHAR_SIZE];
static size_t user_notification_size;

pbio_error_t pbdrv_bluetooth_send_event_notification(pbio_os_state_t *state, pbio_pybricks_event_t event_type, const uint8_t *data, size_t size) {
    PBIO_OS_ASYNC_BEGIN(state);

    if (size + 1 > PBIO_ARRAY_SIZE(user_notification_send_buf)) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Existing user notification ongoing.
    if (user_notification_size) {
        return PBIO_ERROR_BUSY;
    }

    // Copy to local buffer and set size so main thread knows to handle it.
    user_notification_size = size + 1;
    user_notification_send_buf[0] = event_type;
    memcpy(&user_notification_send_buf[1], data, size);
    pbio_os_request_poll();

    // Await until main process has finished sending user data.
    PBIO_OS_AWAIT_WHILE(state, user_notification_size);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

//
// Functions related to connections to peripherals.
//

pbdrv_bluetooth_peripheral_t peripheral_singleton;

const char *pbdrv_bluetooth_peripheral_get_name(void) {
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;
    return peri->name;
}

pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect(pbdrv_bluetooth_peripheral_connect_config_t *config) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    // Can't connect if already connected or already busy.
    if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL) || peri->func) {
        return PBIO_ERROR_BUSY;
    }

    // Initialize operation for handling on the main thread.
    memset(peri, 0, sizeof(pbdrv_bluetooth_peripheral_t));
    peri->config = config;
    peri->func = pbdrv_bluetooth_peripheral_scan_and_connect_func;
    peri->err = PBIO_ERROR_AGAIN;
    pbio_os_timer_set(&peri->timer, config->timeout);
    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_peripheral_disconnect(void) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    // Busy doing something else.
    if (peri->func) {
        return PBIO_ERROR_BUSY;
    }

    // Pass silently for already disconnected.
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        return PBIO_SUCCESS;
    }

    // Initialize operation for handling on the main thread.
    peri->config->notification_handler = NULL;
    peri->func = pbdrv_bluetooth_peripheral_disconnect_func;
    peri->err = PBIO_ERROR_AGAIN;
    pbio_os_request_poll();
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic(pbdrv_bluetooth_peripheral_char_t *characteristic) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        return PBIO_ERROR_NO_DEV;
    }
    if (peri->func) {
        return PBIO_ERROR_BUSY;
    }

    // Initialize operation for handling on the main thread.
    characteristic->handle = 0;
    peripheral_singleton.char_now = characteristic;
    peri->func = pbdrv_bluetooth_peripheral_discover_characteristic_func;
    peri->err = PBIO_ERROR_AGAIN;
    pbio_os_request_poll();
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic(pbdrv_bluetooth_peripheral_char_t *characteristic) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        return PBIO_ERROR_NO_DEV;
    }
    if (peri->func) {
        return PBIO_ERROR_BUSY;
    }

    // Initialize operation for handling on the main thread.
    peripheral_singleton.char_now = characteristic;
    peri->func = pbdrv_bluetooth_peripheral_read_characteristic_func;
    peri->err = PBIO_ERROR_AGAIN;
    pbio_os_request_poll();
    return PBIO_SUCCESS;
}

uint16_t pbdrv_bluetooth_char_write_handle;
uint8_t pbdrv_bluetooth_char_write_data[PBDRV_BLUETOOTH_MAX_CHAR_SIZE];
size_t pbdrv_bluetooth_char_write_size;

pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic(uint16_t handle, const uint8_t *data, size_t size) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        return PBIO_ERROR_NO_DEV;
    }
    if (peri->func) {
        return PBIO_ERROR_BUSY;
    }
    if (size > PBIO_ARRAY_SIZE(pbdrv_bluetooth_char_write_data)) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Initialize operation for handling on the main thread.
    pbdrv_bluetooth_char_write_handle = handle;
    memcpy(pbdrv_bluetooth_char_write_data, data, size);
    pbdrv_bluetooth_char_write_size = size;

    peri->func = pbdrv_bluetooth_peripheral_write_characteristic_func;
    peri->err = PBIO_ERROR_AGAIN;
    pbio_os_request_poll();
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_bluetooth_await_peripheral_command(pbio_os_state_t *state, void *context) {

    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    // If the user is no longer calling this then the operation is no longer
    // of interest and will be cancelled if the active function supports it.
    pbio_os_timer_set(&peri->watchdog, 10);

    return peri->func ? peri->err : PBIO_SUCCESS;
}

void pbdrv_bluetooth_cancel_operation_request(void) {
    // Only some peripheral actions support cancellation.
    peripheral_singleton.cancel = true;
}

//
// Functions related to advertising and scanning.
//

/**
 * Ongoing task used for any of the start/stop advertising/broadcasting functions.
 *
 * Handled by pbdrv_bluetooth_main_thread.
 */
static pbio_os_process_func_t advertising_or_scan_func;
static pbio_error_t advertising_or_scan_err;

pbio_error_t pbdrv_bluetooth_start_advertising(bool start) {

    if (advertising_or_scan_func) {
        return PBIO_ERROR_BUSY;
    }

    // Invalidate broadcast data cache.
    pbdrv_bluetooth_broadcast_data_size = 0;

    // Initialize newly given task.
    advertising_or_scan_err = PBIO_ERROR_AGAIN;
    advertising_or_scan_func = start ?
        pbdrv_bluetooth_start_advertising_func :
        pbdrv_bluetooth_stop_advertising_func;
    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

uint8_t pbdrv_bluetooth_broadcast_data[PBDRV_BLUETOOTH_MAX_ADV_SIZE];
uint8_t pbdrv_bluetooth_broadcast_data_size;

pbio_error_t pbdrv_bluetooth_start_broadcasting(const uint8_t *data, size_t size) {

    if (advertising_or_scan_func) {
        return PBIO_ERROR_BUSY;
    }

    if (size > PBDRV_BLUETOOTH_MAX_ADV_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // This means stop broadcasting.
    if (!data || !size) {
        if (!pbdrv_bluetooth_is_broadcasting) {
            return PBIO_SUCCESS;
        }
        advertising_or_scan_err = PBIO_ERROR_AGAIN;
        advertising_or_scan_func = pbdrv_bluetooth_stop_advertising_func;
        pbio_os_request_poll();
        return PBIO_SUCCESS;
    }

    // Avoid I/O operations if the user tries to broadcast the same data
    // over and over in a tight loop.
    if (pbdrv_bluetooth_is_broadcasting && pbdrv_bluetooth_broadcast_data_size == size && !memcmp(pbdrv_bluetooth_broadcast_data, data, size)) {
        return PBIO_SUCCESS;
    }
    pbdrv_bluetooth_broadcast_data_size = size;
    memcpy(pbdrv_bluetooth_broadcast_data, data, size);

    // Initialize newly given task.
    advertising_or_scan_err = PBIO_ERROR_AGAIN;
    advertising_or_scan_func = pbdrv_bluetooth_start_broadcasting_func;
    pbio_os_request_poll();

    return PBIO_SUCCESS;
}

bool pbdrv_bluetooth_is_broadcasting;
bool pbdrv_bluetooth_is_observing;
pbdrv_bluetooth_start_observing_callback_t pbdrv_bluetooth_observe_callback;

pbio_error_t pbdrv_bluetooth_start_observing(pbdrv_bluetooth_start_observing_callback_t callback) {

    if (advertising_or_scan_func) {
        return PBIO_ERROR_BUSY;
    }

    pbdrv_bluetooth_observe_callback = callback;

    bool should_observe = callback ? true : false;

    if (should_observe == pbdrv_bluetooth_is_observing) {
        return PBIO_SUCCESS;
    }

    // Initialize newly given task.
    advertising_or_scan_err = PBIO_ERROR_AGAIN;
    advertising_or_scan_func = should_observe ? pbdrv_bluetooth_start_observing_func : pbdrv_bluetooth_stop_observing_func;
    pbio_os_request_poll();
    return PBIO_SUCCESS;
}

static bool observe_restart_requested;

void pbdrv_bluetooth_restart_observing_request(void) {
    observe_restart_requested = true;
    pbio_os_request_poll();
}

pbio_error_t pbdrv_bluetooth_await_advertise_or_scan_command(pbio_os_state_t *state, void *context) {
    return advertising_or_scan_func ? advertising_or_scan_err : PBIO_SUCCESS;
}

/**
 * This is the main high level pbdrv/bluetooth thread. It is driven forward by
 * the platform-specific HCI process whenever there is new data to process or
 * when it is ready to send a new command.
 *
 * This somewhat inverted way of running things means that each of the sub
 * threads here don't need to await all hci operations everywhere. They can
 * just prepare to a command them whereas the parent process will send it and
 * have us proceed when it is done.
 */
pbio_error_t pbdrv_bluetooth_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t sub;
    static pbio_os_timer_t timer;
    static pbio_os_timer_t status_timer;
    pbio_error_t err;

    // Shorthand notation accessible throughout.
    bool can_send = pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PYBRICKS);
    pbdrv_bluetooth_peripheral_t *peri = &peripheral_singleton;

    PBIO_OS_ASYNC_BEGIN(state);

    for (;;) {

        pbdrv_bluetooth_is_broadcasting = false;
        pbdrv_bluetooth_is_observing = false;
        observe_restart_requested = false;

        PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_controller_reset(&sub, &timer));

        DEBUG_PRINT("Bluetooth is now off.\n");
        power_change_busy = false;
        pbio_os_request_poll();

        // Bluetooth is now disabled. Await system processes to ask for enable.
        PBIO_OS_AWAIT_UNTIL(state, power_on_requested);
        DEBUG_PRINT("Bluetooth enable requested.\n");

        PBIO_OS_AWAIT(state, &sub, err = pbdrv_bluetooth_controller_initialize(&sub, &timer));
        if (err != PBIO_SUCCESS) {
            DEBUG_PRINT("Initialization failed. Reset and retry.\n");
            continue;
        }

        DEBUG_PRINT("Bluetooth is now on and initialized.\n");
        power_change_busy = false;
        pbio_os_request_poll();

        pbio_os_timer_set(&status_timer, PBDRV_BLUETOOTH_STATUS_UPDATE_INTERVAL);

        // Service scheduled tasks as long as Bluetooth is enabled.
        while (power_on_requested) {

            // REVISIT: Only needed if there is nothing to do
            PBIO_OS_AWAIT_MS(state, &timer, 1);

            // Handle pending status update, if any.
            if (can_send && (status_data_pending || pbio_os_timer_is_expired(&status_timer))) {

                // When a status is pending, cache it here while we write it out,
                // so a new status can be set in the mean time without losing it.
                static uint8_t status_buf[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
                memcpy(status_buf, status_data, PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE);
                status_data_pending = false;

                PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_send_pybricks_value_notification(&sub, status_buf, sizeof(status_buf)));

                pbio_os_timer_set(&status_timer, status_timer.duration);
            }

            // Handle pending stdout, if any.
            if (can_send && lwrb_get_full(&stdout_ring_buf) != 0) {
                stdout_send_busy = true;

                static uint8_t stdout_buf[PBDRV_BLUETOOTH_MAX_CHAR_SIZE] = { PBIO_PYBRICKS_EVENT_WRITE_STDOUT };
                static uint16_t stdout_len;

                stdout_len = lwrb_read(&stdout_ring_buf, &stdout_buf[1], PBDRV_BLUETOOTH_MAX_CHAR_SIZE - 1) + 1;
                PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_send_pybricks_value_notification(&sub, stdout_buf, stdout_len));

                stdout_send_busy = false;
            }

            // Handle pending user value notification, if any.
            if (can_send && user_notification_size) {

                PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_send_pybricks_value_notification(&sub,
                    user_notification_send_buf,
                    user_notification_size));

                user_notification_size = 0;
            }

            // Handle pending advertising/scan enable/disable task, if any.
            if (advertising_or_scan_func) {
                PBIO_OS_AWAIT(state, &sub, advertising_or_scan_err = advertising_or_scan_func(&sub, NULL));
                advertising_or_scan_func = NULL;
            }

            // Handle pending peripheral task, if any.
            if (peri->func) {

                // If currently observing, stop if we need to scan for a peripheral.
                if (pbdrv_bluetooth_is_observing && peri->func == pbdrv_bluetooth_peripheral_scan_and_connect_func) {
                    PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_stop_observing_func(&sub, NULL));
                    observe_restart_requested = true;
                }

                PBIO_OS_AWAIT(state, &sub, peri->err = peri->func(&sub, peri));
                peri->func = NULL;
                peri->cancel = false;
            }

            // Restart if we stopped it temporarily to scan for a peripheral or
            // when externaly requested.
            if (observe_restart_requested) {
                DEBUG_PRINT("Restart observe.\n");
                PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_stop_observing_func(&sub, NULL));
                PBIO_OS_AWAIT(state, &sub, pbdrv_bluetooth_start_observing_func(&sub, NULL));
                observe_restart_requested = false;
            }
        }

        DEBUG_PRINT("Bluetooth disable requested.\n");
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#endif // PBDRV_CONFIG_BLUETOOTH_STM32_CC2640
