// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

/**
 * @addtogroup BluetoothDriver Driver: Bluetooth
 * @{
 */

#ifndef _PBDRV_BLUETOOTH_H_
#define _PBDRV_BLUETOOTH_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>
#include <lego/lwp3.h>

/**
 * BLE characteristic connection identifiers.
 */
typedef enum {
    /* A low energy device connection. */
    PBDRV_BLUETOOTH_CONNECTION_LE,
    /** The Pybricks service. */
    PBDRV_BLUETOOTH_CONNECTION_PYBRICKS,
    /** The Nordic UART service. */
    PBDRV_BLUETOOTH_CONNECTION_UART,
    /** A LEGO Powered Up Handset peripheral connection. */
    PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL,
} pbdrv_bluetooth_connection_t;

/** Data structure that holds context needed for sending BLE notifications. */
typedef struct _pbdrv_bluetooth_send_context_t pbdrv_bluetooth_send_context_t;

/**
 * Callback that is called when sending a notification is done.
 */
typedef void (*pbdrv_bluetooth_send_done_t)(void);

/**
 * Callback that is called when Pybricks BLE characteristic is written to.
 *
 * @param [in]  data        The data that was received.
 * @param [in]  size        The size of @p data in bytes.
 */
typedef pbio_pybricks_error_t (*pbdrv_bluetooth_receive_handler_t)(const uint8_t *data, uint32_t size);

/** Advertisement of scan response match result */
typedef enum {
    /** No match. */
    PBDRV_BLUETOOTH_AD_MATCH_NONE = 0,
    /** Matched the expected value such as device type or manufacturer data. */
    PBDRV_BLUETOOTH_AD_MATCH_VALUE = 1 << 0,
    /** Failed to matched the expected Bluetooth address.*/
    PBDRV_BLUETOOTH_AD_MATCH_ADDRESS = 1 << 1,
    /** A name filter was given and it did NOT match. */
    PBDRV_BLUETOOTH_AD_MATCH_NAME_FAILED = 1 << 2,
} pbdrv_bluetooth_ad_match_result_flags_t;

/**
 * Callback to match an advertisement or scan response.
 *
 * @param [in]  event_type  The type of advertisement.
 * @param [in]  data        The advertisement data.
 * @param [in]  name        The name to match. If NULL, no name filter is applied.
 * @param [in]  addr        The currently detected address if known, else NULL.
 * @param [in]  match_addr  The address to match. If NULL, no address filter is applied.
 * @return                  True if the advertisement matches, false otherwise.
 */
typedef pbdrv_bluetooth_ad_match_result_flags_t (*pbdrv_bluetooth_ad_match_t)
    (uint8_t event_type, const uint8_t *data, const char *name, const uint8_t *addr, const uint8_t *match_addr);

struct _pbdrv_bluetooth_send_context_t {
    /** Callback that is called when the data has been sent. */
    pbdrv_bluetooth_send_done_t done;
    /** The data to be sent. This data must remain valid until @p done is called. */
    const uint8_t *data;
    /** The size of @p data. */
    uint8_t size;
    /** The connection to use. Only characteristics with notify capability are allowed. */
    pbdrv_bluetooth_connection_t connection;
};

/**
 * A GATT value.
 */
typedef struct {
    /** Value handle, little endian. Stored as two bytes to ensure safe packing. */
    uint8_t handle[2];
    /** The size of @p data in bytes. */
    uint8_t size;
    /** The value data. */
    uint8_t data[0];
} pbdrv_bluetooth_value_t;

/**
 * Client characteristic discovery request and resulting handle.
 *
 * These structures are (statically) allocated in the application and are
 * assumed to persist until the discovery is complete.
 */
typedef struct {
    /** Discovered handle. Will remain 0 if not yet found or failed. */
    uint16_t handle;
    /** Highest handle to look for. */
    uint16_t handle_max;
    /** Properties to filter discovered results. Leave 0 for no filtering. */
    uint16_t properties;
    /** The 16-bit UUID. Leave at 0 if 128-bit UUID should be used. */
    uint16_t uuid16;
    /** The 128-bit UUID (big endian), used if uuid16 not set. */
    const uint8_t uuid128[16];
    /** Whether to request enabling notifications after successful discovery. */
    bool request_notification;
    /** Most recently received value. */
    uint8_t value_len;
    uint8_t value[20];
} pbdrv_bluetooth_peripheral_char_t;

/** Peripheral connection options flags. */
typedef enum {
    /** No options. */
    PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_NONE = 0,
    /** Whether to initiate pairing after connecting. */
    PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR = 1 << 0,
    /** Whether to disconnect from the host before connecting to peripheral. */
    PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_DISCONNECT_HOST = 1 << 1,
} pbdrv_bluetooth_peripheral_options_t;

/** Peripheral scan and connection configuration */
typedef struct {
    /** Matcher for advertisement */
    pbdrv_bluetooth_ad_match_t match_adv;
    /** Matcher for scan response */
    pbdrv_bluetooth_ad_match_t match_adv_rsp;
    /** Handler for received notifications after connecting */
    pbdrv_bluetooth_receive_handler_t notification_handler;
    /** Option flags governing connection and pairing */
    pbdrv_bluetooth_peripheral_options_t options;
    /** Timeout before aborting scan and connect. Use 0 for no timeout. */
    uint32_t timeout;
} pbdrv_bluetooth_peripheral_connect_config_t;

/**
 * State of a peripheral that the hub may be connected to, such as a remote.
 */
typedef struct {
    uint16_t con_handle;
    uint8_t status;
    uint8_t bdaddr_type;
    uint8_t bdaddr[6];
    char name[20];
    /** Handle to the characteristic currently being discovered. */
    pbdrv_bluetooth_peripheral_char_t *char_now;
    /** Scan and connect configuration. */
    pbdrv_bluetooth_peripheral_connect_config_t *config;
    /** Currently ongoing peripheral function. */
    pbio_os_process_func_t func;
    /** Most recent result of calling above function from main process. */
    pbio_error_t err;
    /** Timer for above operation. */
    pbio_os_timer_t timer;
    /** Watchdog for above operation so it can be cancelled on inactivity. */
    pbio_os_timer_t watchdog;
    /** Cancellation requested */
    bool cancel;
} pbdrv_bluetooth_peripheral_t;

/** Advertisement types. */
typedef enum {
    // NB: the numeric values come from the Bluetooth spec - do not change!

    /** Undirected, scannable, connectable. */
    PBDRV_BLUETOOTH_AD_TYPE_ADV_IND = 0,
    /** Directed, scannable, connectable. */
    PBDRV_BLUETOOTH_AD_TYPE_ADV_DIRECT_IND = 1,
    /** Undirected, scannable, non-connectable. */
    PBDRV_BLUETOOTH_AD_TYPE_ADV_SCAN_IND = 2,
    /** Undirected, non-scannable, non-connectable. */
    PBDRV_BLUETOOTH_AD_TYPE_ADV_NONCONN_IND = 3,
    /** Scan response. */
    PBDRV_BLUETOOTH_AD_TYPE_SCAN_RSP = 4,
} pbdrv_bluetooth_ad_type_t;

/** Advertisement data types. */
typedef enum {
    PBDRV_BLUETOOTH_AD_DATA_TYPE_FLAGS = 0x01,
    PBDRV_BLUETOOTH_AD_DATA_TYPE_16_BIT_SERV_UUID_COMPLETE_LIST = 0x03,
    PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_INCOMPLETE_LIST = 0x06,
    PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_COMPLETE_LIST = 0x07,
    PBDRV_BLUETOOTH_AD_DATA_TYPE_TX_POWER_LEVEL = 0x0A,
    PBDRV_BLUETOOTH_AD_DATA_TYPE_APPEARANCE = 0x19,
    PBDRV_BLUETOOTH_AD_DATA_TYPE_MANUFACTURER_DATA = 0xFF,
} pbdrv_bluetooth_ad_data_type_t;

/** Characteristic size */
typedef enum {
    PBDRV_BLUETOOTH_CHAR_UUID_SIZE_16 = 0x01,
    PBDRV_BLUETOOTH_CHAR_UUID_SIZE_128 = 0x02,
} pbdrv_bluetooth_char_size_t;

/**
 * Callback called when advertising data is received.
 * @param [in]  type    The advertisement type.
 * @param [in]  data    The advertising data.
 * @param [in]  length  The length of @p data in bytes.
 * @param [in]  rssi    The RSSI value for the event.
 */
typedef void (*pbdrv_bluetooth_start_observing_callback_t)(pbdrv_bluetooth_ad_type_t type, const uint8_t *data, uint8_t length, int8_t rssi);

#ifdef PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE
#if PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE < 23 || PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE > 515
#error PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE out of range
#endif
#define PBDRV_BLUETOOTH_MAX_MTU_SIZE PBDRV_CONFIG_BLUETOOTH_MAX_MTU_SIZE
#else
/** The maximum allowable MTU size for this chip. */
#define PBDRV_BLUETOOTH_MAX_MTU_SIZE 23
#endif

#if PBDRV_CONFIG_BLUETOOTH

//
// General purpose functions: initialization and power/connected state.
//

/**
 * Initializes the Bluetooth driver.
 */
void pbdrv_bluetooth_init(void);

/**
 * Deinitializes the Bluetooth driver.
 */
void pbdrv_bluetooth_deinit(void);

/**
 * Gets the bluetooth hub name.
 */
const char *pbdrv_bluetooth_get_hub_name(void);

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
 * Tests if a central is connected to the Bluetooth chip.
 * @param [in]  connection  The type of connection of interest.
 * @return                  True if the requested connection type is present,
 *                          otherwise false.
 */
bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection);

/**
 * Registers a callback that will be called when Pybricks data is received via a
 * characteristic write.
 *
 * @param [in]  handler     The function that will be called.
 */
void pbdrv_bluetooth_set_receive_handler(pbdrv_bluetooth_receive_handler_t handler);


//
// Functions related to sending value notifications (stdout, status, app data).
//
// These are all transferred over the air in the same way, but the interfaces
// below differ in terms of how the data is scheduled and buffered or awaited.
//

/**
 * Schedules Pybricks status to be sent via a characteristic notification.
 *
 * The data length is always ::PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE.
 */
void pbdrv_bluetooth_schedule_status_update(const uint8_t *status_msg);

/**
 * Queues data to be transmitted via Bluetooth serial port.
 *
 * @param data  [in]        The data to be sent.
 * @param size  [in, out]   The size of @p data in bytes. After return, @p size
 *                          contains the number of bytes actually written.
 * @return                  ::PBIO_SUCCESS if some @p data was queued,
 *                          ::PBIO_ERROR_AGAIN if no @p data could be
 *                          queued at this time (e.g. buffer is full),
 *                          ::PBIO_ERROR_INVALID_OP if there is not an
 *                          active Bluetooth connection or ::PBIO_ERROR_NOT_SUPPORTED
 *                          if this platform does not support Bluetooth.
 */
pbio_error_t pbdrv_bluetooth_tx(const uint8_t *data, uint32_t *size);

/**
 * Gets the number of bytes that can be queued for transmission via Bluetooth.
 *
 * If there is no connection, this will return ::UINT32_MAX.
 *
 * @returns The number of bytes that can be queued.
 */
uint32_t pbdrv_bluetooth_tx_available(void);

/**
 * Tests if the Tx queue is empty and all data has been sent over the air.
 *
 * If there is no connection, this will always return @c true.
 *
 * @returns @c true if the condition is met, otherwise @c false.
 */
bool pbdrv_bluetooth_tx_is_idle(void);

/**
 * Sends a value notification and await it.
 *
 * Uses the same mechanism as stdout or status events, but is user-awaitable.
 *
 * This does not not use a ringbuffer. Await operation before sending more.
 *
 * @param [in] state    Protothread state.
 * @param [in] event    Event type (status, stdout, or app data).
 * @param [in] data     Data to send.
 * @param [in] size     Data size, not counting event type byte.
 * @return              ::PBIO_SUCCESS on completion.
 *                      ::PBIO_ERROR_INVALID_OP if there is no connection.
 *                      ::PBIO_ERROR_AGAIN while awaiting.
 *                      ::PBIO_ERROR_BUSY if this operation is already ongoing.
 *                      ::PBIO_ERROR_INVALID_ARG if @p size is too large.
 */
pbio_error_t pbdrv_bluetooth_send_event_notification(pbio_os_state_t *state, pbio_pybricks_event_t event, const uint8_t *data, size_t size);

//
// Functions related to connections to peripherals.
//

/**
 * Gets the name of the connected peripheral.
 *
 * @return  The name of the connected peripheral. May not be set.
 */
const char *pbdrv_bluetooth_peripheral_get_name(void);

/**
 * Starts scanning for a BLE device and connects to it.
 *
 * @param [in]  config   Scan and connect configuration.
 * @return               ::PBIO_SUCCESS if the operation was scheduled.
 *                       ::PBIO_ERROR_BUSY if already connected or another peripheral operation is ongoing.
 */
pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect(pbdrv_bluetooth_peripheral_connect_config_t *config);

/**
 * Disconnect from the peripheral.
 *
 * @param [in]  state          Protothread state.
 * @return                     ::PBIO_SUCCESS if disconnection schefuled or when already disconnected.
 *                             ::PBIO_ERROR_BUSY if another peripheral operation is ongoing.
 */
pbio_error_t pbdrv_bluetooth_peripheral_disconnect(void);

/**
 * Find a characteristic by UUID and properties.
 *
 * If found, the value handle in the characteristic is set.
 *
 * @param [in]  state          Protothread state.
 * @param [in]  characteristic The characteristic to discover.
 * @return                     ::PBIO_SUCCESS when successfully scheuled..
 *                             ::PBIO_ERROR_BUSY if another peripheral operation is ongoing.
 *                             ::PBIO_ERROR_NO_DEV if no peripheral is connected.
 */
pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic(pbdrv_bluetooth_peripheral_char_t *characteristic);

/**
 * Read a characteristic.
 *
 * @param [in]  characteristic The characteristic to read.
 * @return                     ::PBIO_SUCCESS if the read was scheduled.
 *                             ::PBIO_ERROR_NO_DEV if not connected to a peripheral.
 *                             ::PBIO_ERROR_BUSY if another operation is ongoing.
 */
pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic(pbdrv_bluetooth_peripheral_char_t *characteristic);

/**
 * Write a value to a peripheral characteristic without response.
 *
 * The write is queued for transmission and does not await completion.
 *
 * @param [in]  handle     The handle of the characteristic value to write.
 * @param [in]  data       The data to write.
 * @param [in]  size       The size of @p data in bytes.
 * @return                 ::PBIO_SUCCESS if message successfully scheduled.
 *                         ::PBIO_ERROR_NO_DEV if not connected to a peripheral.
 *                         ::PBIO_ERROR_BUSY if another peripheral operation is ongoing.
 *                         ::PBIO_ERROR_INVALID_ARG if @p size is too big.
 */
pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic(uint16_t handle, const uint8_t *data, size_t size);

/**
 * Awaits for a task associated with a peripheral to complete. Used to await
 * characteristic discover/read/write, scan-and-connect, or disconnect.
 *
 * @param [in]  state          Protothread state. Not used in all implementations.
 * @param [in]  context        Not used.
 * @return                     ::PBIO_SUCCESS on completion.
 *                             ::PBIO_ERROR_AGAIN while awaiting.
 *                             or a thread specific error code if the operation failed.
 */
pbio_error_t pbdrv_bluetooth_await_peripheral_command(pbio_os_state_t *state, void *context);

/**
 * Requests active Bluetooth tasks to be cancelled. It is up to the task
 * implementation to respect or ignore it. The task should still be awaited
 * with ::pbdrv_bluetooth_await_advertise_or_scan_command or
 * with ::pbdrv_bluetooth_await_peripheral_command. Cancelling just allows
 * some commands to exit earlier.
 */
void pbdrv_bluetooth_cancel_operation_request(void);

//
// Functions related to advertising and scanning.
//

/**
 * Starts or stops the advertising process. Configures
 * advertisements and tells the Bluetooth chip to start advertising.
 * Advertising should automatically stop when a connection is made.
 *
 * @param [in] start    @c true for start, @c false for stop.
 * @return              ::PBIO_SUCCESS if operation was scheduled.
 *                      ::PBIO_ERROR_BUSY if an advertising or scan initialization
 *                      command is already in progress.
 */
pbio_error_t pbdrv_bluetooth_start_advertising(bool start);

/**
 * Starts broadcasting undirected, non-connectable, non-scannable advertisement
 * data.
 *
 * Call again to update the advertising data if needed.
 *
 * The advertising data must follow the Bluetooth specification. The length
 * is validated, but the data itself is not.
 *
 * Setting @p data to NULL or @p size to 0 stops broadcasting.
 *
 * @param [in]  data    The advertising data.
 * @param [in]  size    The length of @p data in bytes.
 * @return              ::PBIO_SUCCESS if operation was scheduled,
 *                      ::PBIO_ERROR_BUSY if another advertising/broadcast/scan is ongoing.
 *                      ::PBIO_ERROR_INVALID_ARG if @p size is invalid.
 */
pbio_error_t pbdrv_bluetooth_start_broadcasting(const uint8_t *data, size_t size);

/**
 * Starts observing, non-connectable, non-scannable advertisements.
 *
 * It is safe to call this function multiple times without stopping first.
 *
 * @param [in]  callback    A callback that is called each time advertising data is received.
 *                          Choose NULL to stop observing.
 * @return                  ::PBIO_SUCCESS if operation was scheduled,
 *                          ::PBIO_ERROR_BUSY if another advertising/broadcast/scan command is ongoing.
 */
pbio_error_t pbdrv_bluetooth_start_observing(pbdrv_bluetooth_start_observing_callback_t callback);

/**
 * Request to restart observing if it is active.
 *
 * Used on device that require occasionally restarting observing to keep it active.
 */
void pbdrv_bluetooth_restart_observing_request(void);

/**
 * Awaits for the advertising/broadcast/scan/observe call to complete.
 *
 * This awaits only the (HCI) command that starts or stops scanning
 * or advertising. This does not await for the actual scan to complete.
 *
 * @param [in]  state          Protothread state. Not used in all implementations.
 * @param [in]  context        Not used.
 * @return                     ::PBIO_SUCCESS on completion.
 *                             ::PBIO_ERROR_AGAIN while awaiting.
 *                             or an thread specific error code if the operation failed.
 */
pbio_error_t pbdrv_bluetooth_await_advertise_or_scan_command(pbio_os_state_t *state, void *context);

/**
 * Awaits user activity to complete, usually called during cleanup after running
 * a user program. This will disconnect from the peripheral and stop scanning
 * and advertising.
 *
 * @param [in]  state          Protothread state.
 * @param [in]  timer          Timer used to give up if this takes too long.
 * @return                     ::PBIO_SUCCESS on completion.
 *                             ::PBIO_ERROR_AGAIN while awaiting.
 *                             ::PBIO_ERROR_TIMEDOUT if the timer expired.
 */
pbio_error_t pbdrv_bluetooth_close_user_tasks(pbio_os_state_t *state, pbio_os_timer_t *timer);

#else // PBDRV_CONFIG_BLUETOOTH

static inline void pbdrv_bluetooth_init(void) {
}

static inline void pbdrv_bluetooth_deinit(void) {
}

static inline const char *pbdrv_bluetooth_get_hub_name(void) {
    return "Pybricks Hub";
}

static inline const char *pbdrv_bluetooth_get_fw_version(void) {
    return "";
}

static inline bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    return false;
}

static inline void pbdrv_bluetooth_set_receive_handler(pbdrv_bluetooth_receive_handler_t handler) {
}

static inline void pbdrv_bluetooth_schedule_status_update(const uint8_t *status_msg) {
}

static inline pbio_error_t pbdrv_bluetooth_tx(const uint8_t *data, uint32_t *size) {
    if (size) {
        *size = 0;
    }
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline uint32_t pbdrv_bluetooth_tx_available(void) {
    return UINT32_MAX;
}

static inline bool pbdrv_bluetooth_tx_is_idle(void) {
    return true;
}

static inline pbio_error_t pbdrv_bluetooth_send_event_notification(
    pbio_os_state_t *state, pbio_pybricks_event_t event, const uint8_t *data, size_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline const char *pbdrv_bluetooth_peripheral_get_name(void) {
    return NULL;
}

static inline pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect(pbdrv_bluetooth_peripheral_connect_config_t *config) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_peripheral_disconnect(void) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic(pbdrv_bluetooth_peripheral_char_t *characteristic) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic(
    pbdrv_bluetooth_peripheral_char_t *characteristic) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic(uint16_t handle, const uint8_t *data, size_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_await_peripheral_command(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_cancel_operation_request(void) {
}

static inline pbio_error_t pbdrv_bluetooth_start_advertising(bool start) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_start_broadcasting(const uint8_t *data, size_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_start_observing(
    pbdrv_bluetooth_start_observing_callback_t callback) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_await_advertise_or_scan_command(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_bluetooth_close_user_tasks(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    // Should not hold up systems without Bluetooth.
    return PBIO_SUCCESS;
}


#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @} */
