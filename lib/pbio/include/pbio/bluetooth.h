// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

/**
 * @addtogroup BluetoothDriver Driver: Bluetooth
 * @{
 */

#ifndef _PBIO_BLUETOOTH_H_
#define _PBIO_BLUETOOTH_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>
#include <lego/lwp3.h>

#define PBIO_BLUETOOTH_STATUS_UPDATE_INTERVAL (500)
#define PBIO_BLUETOOTH_MAX_CHAR_SIZE 20
#define PBIO_BLUETOOTH_MAX_ADV_SIZE 31
#define PBIO_BLUETOOTH_ATT_HEADER_SIZE (3)

/**
 * Callback to match an advertisement or scan response.
 *
 * @param [in]  user        The user of this peripheral, usually a high-level object.
 * @param [in]  data        The advertisement data.
 * @param [in]  length      The advertisement data size.
 * @return                  True if the advertisement matches, false otherwise.
 */
typedef bool (*pbio_bluetooth_advertising_callback_t)(void *user, const uint8_t *data, uint8_t length);

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
    /** The 128-bit UUID (big endian), used if uuid16 not set. Should be persistent. */
    const uint8_t *uuid128;
    /** Whether to request enabling notifications after successful discovery. */
    bool request_notification;
} pbio_bluetooth_peripheral_char_discovery_t;

/** Peripheral connection options flags. */
typedef enum {
    /** No options. */
    PBIO_BLUETOOTH_PERIPHERAL_OPTIONS_NONE = 0,
    /** Whether to initiate pairing after connecting. */
    PBIO_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR = 1 << 0,
    /** Whether to disconnect from the host before connecting to peripheral. */
    PBIO_BLUETOOTH_PERIPHERAL_OPTIONS_DISCONNECT_HOST = 1 << 1,
} pbio_bluetooth_peripheral_options_t;

typedef struct _pbio_bluetooth_peripheral_t pbio_bluetooth_peripheral_t;

/**
 * Callback that is called when a peripheral sends a notification.
 *
 * @param [in]  user        The user of this peripheral, usually a high-level object.
 * @param [in]  data        The data that was received.
 * @param [in]  size        The size of @p data in bytes.
 */
typedef void (*pbio_bluetooth_peripheral_notification_handler_t)(void *user, const uint8_t *data, uint32_t size);

/** Peripheral scan and connection configuration */
typedef struct {
    /** Matcher for advertisement */
    pbio_bluetooth_advertising_callback_t match_adv;
    /** Matcher for scan response */
    pbio_bluetooth_advertising_callback_t match_adv_rsp;
    /** Handler for received notifications after connecting */
    pbio_bluetooth_peripheral_notification_handler_t notification_handler;
    /** Option flags governing connection and pairing */
    pbio_bluetooth_peripheral_options_t options;
    /** Timeout before aborting scan and connect. Use 0 for no timeout. */
    uint32_t timeout;
    /** Last matching advertisement data for this peripheral. */
    uint8_t match_adv_data[PBIO_BLUETOOTH_MAX_ADV_SIZE];
    /** Last matching advertisement data length for this peripheral. */
    uint8_t match_adv_data_len;
    /** Last matching advertisement response data for this peripheral. */
    uint8_t match_adv_rsp_data[PBIO_BLUETOOTH_MAX_ADV_SIZE];
    /** Last matching advertisement response data length for this peripheral. */
    uint8_t match_adv_rsp_data_len;
} pbio_bluetooth_peripheral_connect_config_t;

/** Platform-specific state needed to operate the peripheral. */
typedef struct _pbio_bluetooth_peripheral_platform_state_t pbio_bluetooth_peripheral_platform_state_t;

/**
 * State of a peripheral that the hub may be connected to, such as a remote.
 */
struct _pbio_bluetooth_peripheral_t {
    /**
     * Optional reference to higher-level user of this peripheral.
     */
    void *user;
    uint16_t con_handle;
    uint8_t status;
    uint8_t bdaddr_type;
    uint8_t bdaddr[6];
    char name[20];
    /** The characteristic currently being discovered. */
    pbio_bluetooth_peripheral_char_discovery_t char_disc;
    /** Scan and connect configuration. */
    pbio_bluetooth_peripheral_connect_config_t config;
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
    /** Platform-specific state needed to operate the peripheral. */
    pbio_bluetooth_peripheral_platform_state_t *platform_state;
    /**
     * Handle of the characteristic to read or write (used by write_func and read_func).
     */
    uint16_t char_handle;
    /**
     * Buffer for reading/writing characteristic value (used by write_func and read_func).
     */
    uint8_t char_data[PBIO_BLUETOOTH_MAX_CHAR_SIZE];
    /**
     * Size of the data to write or data read (used by write_func and read_func).
     */
    size_t char_size;
};

/** Advertisement types. */
typedef enum {
    // NB: the numeric values come from the Bluetooth spec - do not change!

    /** Undirected, scannable, connectable. */
    PBIO_BLUETOOTH_AD_TYPE_ADV_IND = 0,
    /** Directed, scannable, connectable. */
    PBIO_BLUETOOTH_AD_TYPE_ADV_DIRECT_IND = 1,
    /** Undirected, scannable, non-connectable. */
    PBIO_BLUETOOTH_AD_TYPE_ADV_SCAN_IND = 2,
    /** Undirected, non-scannable, non-connectable. */
    PBIO_BLUETOOTH_AD_TYPE_ADV_NONCONN_IND = 3,
    /** Scan response. */
    PBIO_BLUETOOTH_AD_TYPE_SCAN_RSP = 4,
} pbio_bluetooth_ad_type_t;

/** Advertisement data types. */
typedef enum {
    PBIO_BLUETOOTH_AD_DATA_TYPE_FLAGS = 0x01,
    PBIO_BLUETOOTH_AD_DATA_TYPE_16_BIT_SERV_UUID_COMPLETE_LIST = 0x03,
    PBIO_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_INCOMPLETE_LIST = 0x06,
    PBIO_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_COMPLETE_LIST = 0x07,
    PBIO_BLUETOOTH_AD_DATA_TYPE_TX_POWER_LEVEL = 0x0A,
    PBIO_BLUETOOTH_AD_DATA_TYPE_APPEARANCE = 0x19,
    PBIO_BLUETOOTH_AD_DATA_TYPE_MANUFACTURER_DATA = 0xFF,
} pbio_bluetooth_ad_data_type_t;

/** Characteristic size */
typedef enum {
    PBIO_BLUETOOTH_CHAR_UUID_SIZE_16 = 0x01,
    PBIO_BLUETOOTH_CHAR_UUID_SIZE_128 = 0x02,
} pbio_bluetooth_char_size_t;

/**
 * Callback called when advertising data is received.
 * @param [in]  type    The advertisement type.
 * @param [in]  data    The advertising data.
 * @param [in]  length  The length of @p data in bytes.
 * @param [in]  rssi    The RSSI value for the event.
 */
typedef void (*pbio_bluetooth_start_observing_callback_t)(pbio_bluetooth_ad_type_t type, const uint8_t *data, uint8_t length, int8_t rssi);

/**
 * A single result from an inquiry scan.
 */
typedef struct {
    /** Bluetooth device address. */
    uint8_t bdaddr[6];
    /** Received signal strength indicator. */
    int8_t rssi;
    /** Device name. */
    char name[249];
    /** Class of device. */
    uint32_t class_of_device;
} pbio_bluetooth_inquiry_result_t;

#if PBIO_CONFIG_BLUETOOTH

//
// General purpose functions: initialization and power/connected state.
//

/**
 * Deinitializes the Bluetooth driver.
 */
void pbio_bluetooth_deinit(void);

/**
 * Gets the bluetooth hub name. REVISIT: Move to sys/host.
 */
const char *pbdrv_bluetooth_get_hub_name(void);

//
// Functions related to connections to peripherals.
//

/**
 * Gets an available (free and unconnected) peripheral instance.
 *
 * @param [out] peripheral   Pointer to the peripheral instance if found.
 * @param [in]  user         Optional user reference to associate with the peripheral.
 * @return                  ::PBIO_SUCCESS if a peripheral instance is available.
 *                          ::PBIO_ERROR_NO_DEV if no peripheral instance is available.
 *                          ::PBIO_ERROR_BUSY if all peripheral instances are in use.
 */
pbio_error_t pbio_bluetooth_peripheral_get_available(pbio_bluetooth_peripheral_t **peripheral, void *user);

/**
 * Gets an matching connected peripheral if available,
 *
 * @param [out] peripheral   Pointer to the peripheral instance if found.
 * @param [in]  user         Optional user reference to associate with the peripheral.
 * @param [in]  config       Config as in scan and connect, used to match previously connected device.
 * @return                   ::PBIO_SUCCESS if a peripheral instance is connected and available.
 *                           ::PBIO_ERROR_NO_DEV if no matching peripheral instance connected or is available.
 */
pbio_error_t pbio_bluetooth_peripheral_get_connected(pbio_bluetooth_peripheral_t **peripheral, void *user, pbio_bluetooth_peripheral_connect_config_t *config);
/**
 * Releases a peripheral instance for reuse by another user.
 *
 * @param [in]  peripheral   The peripheral instance to free.
 * @param [in]  user         The user reference that was used to claim the peripheral.
 */
void pbio_bluetooth_peripheral_release(pbio_bluetooth_peripheral_t *peripheral, void *user);

/**
 * Gets the name of the connected peripheral.
 *
 * @param [in]  peripheral     The peripheral to use.
 * @return  The name of the connected peripheral. May not be set.
 */
const char *pbio_bluetooth_peripheral_get_name(pbio_bluetooth_peripheral_t *peripheral);

/**
 * Starts scanning for a BLE device and connects to it.
 *
 * @param [in]  peripheral     The peripheral to use.
 * @param [in]  config   Scan and connect configuration.
 * @return               ::PBIO_SUCCESS if the operation was scheduled.
 *                       ::PBIO_ERROR_BUSY if already connected or another peripheral operation is ongoing.
 */
pbio_error_t pbio_bluetooth_peripheral_scan_and_connect(pbio_bluetooth_peripheral_t *peripheral, pbio_bluetooth_peripheral_connect_config_t *config);

/**
 * Disconnect from the peripheral.
 *
 * @param [in]  peripheral     The peripheral to use.
 * @return                     ::PBIO_SUCCESS if disconnection schefuled or when already disconnected.
 *                             ::PBIO_ERROR_BUSY if another peripheral operation is ongoing.
 */
pbio_error_t pbio_bluetooth_peripheral_disconnect(pbio_bluetooth_peripheral_t *peripheral);

/**
 * Find a characteristic by UUID and properties.
 *
 * If found, the value handle in the characteristic is set.
 *
 * @param [in]  peripheral     The peripheral to use.
 * @param [in]  characteristic The characteristic to discover.
 * @return                     ::PBIO_SUCCESS when successfully scheuled..
 *                             ::PBIO_ERROR_BUSY if another peripheral operation is ongoing.
 *                             ::PBIO_ERROR_NO_DEV if no peripheral is connected.
 */
pbio_error_t pbio_bluetooth_peripheral_discover_characteristic(pbio_bluetooth_peripheral_t *peripheral, pbio_bluetooth_peripheral_char_discovery_t *characteristic);

/**
 * Gets the result of the last characteristic discovery.
 *
 * @param [in]  peri    The peripheral to use.
 * @return              The handle of the discovered characteristic, or 0 if not found.
 */
uint16_t pbio_bluetooth_peripheral_discover_characteristic_get_result(pbio_bluetooth_peripheral_t *peri);

/**
 * Read a characteristic.
 *
 * @param [in]  peripheral     The peripheral to use.
 * @param [in]  handle         Handle of the characteristic to read.
 * @return                     ::PBIO_SUCCESS if the read was scheduled.
 *                             ::PBIO_ERROR_NO_DEV if not connected to a peripheral.
 *                             ::PBIO_ERROR_BUSY if another operation is ongoing.
 */
pbio_error_t pbio_bluetooth_peripheral_read_characteristic(pbio_bluetooth_peripheral_t *peripheral, uint16_t handle);

/**
 * Write a value to a peripheral characteristic without response.
 *
 * The write is queued for transmission and does not await completion.
 *
 * @param [in]  peripheral The peripheral to write to.
 * @param [in]  handle     The handle of the characteristic value to write.
 * @param [in]  data       The data to write.
 * @param [in]  size       The size of @p data in bytes.
 * @return                 ::PBIO_SUCCESS if message successfully scheduled.
 *                         ::PBIO_ERROR_NO_DEV if not connected to a peripheral.
 *                         ::PBIO_ERROR_BUSY if another peripheral operation is ongoing.
 *                         ::PBIO_ERROR_INVALID_ARG if @p size is too big.
 */
pbio_error_t pbio_bluetooth_peripheral_write_characteristic(pbio_bluetooth_peripheral_t *peripheral, uint16_t handle, const uint8_t *data, size_t size);

/**
 * Awaits for a task associated with a peripheral to complete. Used to await
 * characteristic discover/read/write, scan-and-connect, or disconnect.
 *
 * @param [in]  state          Protothread state. Not used in all implementations.
 * @param [in]  context        The peripheral.
 * @return                     ::PBIO_SUCCESS on completion.
 *                             ::PBIO_ERROR_AGAIN while awaiting.
 *                             or a thread specific error code if the operation failed.
 */
pbio_error_t pbio_bluetooth_await_peripheral_command(pbio_os_state_t *state, void *context);

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
pbio_error_t pbio_bluetooth_start_advertising(bool start);

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
pbio_error_t pbio_bluetooth_start_broadcasting(const uint8_t *data, size_t size);

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
pbio_error_t pbio_bluetooth_start_observing(pbio_bluetooth_start_observing_callback_t callback);

/**
 * Request to restart observing if it is active.
 *
 * Used on device that require occasionally restarting observing to keep it active.
 */
void pbio_bluetooth_restart_observing_request(void);

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
pbio_error_t pbio_bluetooth_await_advertise_or_scan_command(pbio_os_state_t *state, void *context);

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
pbio_error_t pbio_bluetooth_close_user_tasks(pbio_os_state_t *state, pbio_os_timer_t *timer);

/**
 * Starts a classic Bluetooth inquiry scan.
 *
 * @param [in] results                Array to store results.
 * @param [in] results_count          Number of results found.
 * @param [in] results_count_max      Maximum number of results to find. Will
 *                                    stop if externally reset to 0.
 * @param [in] duration_ms            Duration of the inquiry scan in milliseconds.
 *                                    It will be internally rounded to the nearest
 *                                    supported nonzero value.
 */
pbio_error_t pbio_bluetooth_start_inquiry_scan(pbio_bluetooth_inquiry_result_t *results, uint32_t *results_count, uint32_t *results_count_max, uint32_t duration_ms);

/**
 * Awaits for the classic Bluetooth inquiry scan to complete.
 *
 * @param [in]  state          Protothread state.
 * @param [in]  context        Not used.
 * @return                     ::PBIO_SUCCESS on completion.
 *                             ::PBIO_ERROR_AGAIN while awaiting.
 *                             or an thread specific error code if the operation failed.
 */
pbio_error_t pbio_bluetooth_await_classic_task(pbio_os_state_t *state, void *context);

#else // PBIO_CONFIG_BLUETOOTH

static inline void pbio_bluetooth_deinit(void) {
}

static inline const char *pbdrv_bluetooth_get_hub_name(void) {
    return "";
}

static inline pbio_error_t pbio_bluetooth_peripheral_get_available(pbio_bluetooth_peripheral_t **peripheral, void *user) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_peripheral_get_connected(pbio_bluetooth_peripheral_t **peripheral, void *user, pbio_bluetooth_peripheral_connect_config_t *config) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_bluetooth_peripheral_release(pbio_bluetooth_peripheral_t *peripheral, void *user) {
}

static inline const char *pbio_bluetooth_peripheral_get_name(pbio_bluetooth_peripheral_t *peripheral) {
    return "";
}

static inline pbio_error_t pbio_bluetooth_peripheral_scan_and_connect(pbio_bluetooth_peripheral_t *peripheral, pbio_bluetooth_peripheral_connect_config_t *config) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_peripheral_disconnect(pbio_bluetooth_peripheral_t *peripheral) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_peripheral_discover_characteristic(pbio_bluetooth_peripheral_t *peripheral, pbio_bluetooth_peripheral_char_discovery_t *characteristic) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline uint16_t pbio_bluetooth_peripheral_discover_characteristic_get_result(pbio_bluetooth_peripheral_t *peri) {
    return 0;
}

static inline pbio_error_t pbio_bluetooth_peripheral_read_characteristic(pbio_bluetooth_peripheral_t *peripheral, uint16_t handle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_peripheral_write_characteristic(pbio_bluetooth_peripheral_t *peripheral, uint16_t handle, const uint8_t *data, size_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_await_peripheral_command(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_start_advertising(bool start) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_start_broadcasting(const uint8_t *data, size_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_start_observing(pbio_bluetooth_start_observing_callback_t callback) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_bluetooth_restart_observing_request(void) {
}

static inline pbio_error_t pbio_bluetooth_await_advertise_or_scan_command(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_close_user_tasks(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_start_inquiry_scan(pbio_bluetooth_inquiry_result_t *results, uint32_t *results_count, uint32_t *results_count_max, uint32_t duration_ms) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_bluetooth_await_classic_task(pbio_os_state_t *state, void *context) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_BLUETOOTH

#endif // _PBIO_BLUETOOTH_H_

/** @} */
