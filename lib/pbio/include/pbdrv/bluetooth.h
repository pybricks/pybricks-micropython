// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

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
#include <pbio/protocol.h>
#include <pbio/task.h>

#include <lego_lwp3.h>

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
 * Callback that is called when a Bluetooth event occurs.
 */
typedef void (*pbdrv_bluetooth_on_event_t)(void);

/**
 * Callback that is called when sending a notification is done.
 */
typedef void (*pbdrv_bluetooth_send_done_t)(void);

/**
 * Callback that is called when BLE characteristic is written to.
 *
 * @param [in]  connection  The characteristic that was written.
 * @param [in]  data        The data that was received.
 * @param [in]  size        The size of @p data in bytes.
 */
typedef pbio_pybricks_error_t (*pbdrv_bluetooth_receive_handler_t)(pbdrv_bluetooth_connection_t connection, const uint8_t *data, uint32_t size);

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


/** Peripheral connection options. */
typedef enum {
    /** No options. */
    PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_NONE = 0,
    /** Whether to initiate pairing after connecting. */
    PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR = 1 << 0,
    /** Whether to disconnect from the host before connecting to peripheral. */
    PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_DISCONNECT_HOST = 1 << 1,
} pbdrv_bluetooth_peripheral_options_t;

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
    pbdrv_bluetooth_ad_match_t match_adv;
    pbdrv_bluetooth_ad_match_t match_adv_rsp;
    pbdrv_bluetooth_receive_handler_t notification_handler;
    pbdrv_bluetooth_peripheral_options_t options;
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

/**
 * Initializes the Bluetooth driver.
 */
void pbdrv_bluetooth_init(void);

/**
 * Turns the Bluetooth chip power on or off.
 * @param [in]  on      If true, turns power on, otherwise turns power off.
 */
void pbdrv_bluetooth_power_on(bool on);

/**
 * Tests if the Bluetooth chip is powered on and is ready to receive commands.
 * @return              True if the Bluetooth chip is ready, otherwise false.
 */
bool pbdrv_bluetooth_is_ready(void);

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
 * Queues a task that does nothing. When this task completes, all tasks
 * before it will have finished or failed.
 */
void pbdrv_bluetooth_queue_noop(pbio_task_t *task);

/**
 * Starts the advertising process, including configuring advertisements and
 * telling the Bluetooth chip to start advertising. Advertising should
 * automatically stop when a connection is made.
 */
void pbdrv_bluetooth_start_advertising(void);

/**
 * Stops the advertising process.
 *
 * This only needs to be called when no central connects to the hub.
 */
void pbdrv_bluetooth_stop_advertising(void);

/**
 * Tests if a central is connected to the Bluetooth chip.
 * @param [in]  connection  The type of connection of interest.
 * @return                  True if the requested connection type is present,
 *                          otherwise false.
 */
bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection);

/**
 * Registers a callback that is called when Bluetooth event occurs.
 *
 * @param [in]  on_event   The callback.
 */
void pbdrv_bluetooth_set_on_event(pbdrv_bluetooth_on_event_t on_event);

/**
 * Requests for @p data to be sent via a characteristic notification.
 *
 * It is up to the caller to verify that notifications are enabled and
 * that any previous notification request is done before calling this function.
 *
 * @param [in]  context     The data to be sent and where to send it.
 */
void pbdrv_bluetooth_send(pbdrv_bluetooth_send_context_t *context);

/**
 * Registers a callback that will be called when data is received via a
 * characteristic write.
 *
 * @param [in]  handler     The function that will be called.
 */
void pbdrv_bluetooth_set_receive_handler(pbdrv_bluetooth_receive_handler_t handler);

/**
 * Starts scanning for a BLE device and connects to it.
 *
 * @param [in]  task           The task that is used to wait for completion.
 * @param [in]  match_adv      Callback to match the advertisement data during scan.
 * @param [in]  match_adv_rsp  Callback to match the advertisement response data during scan.
 * @param [in]  notification_handler  Callback to handle notifications from the peripheral.
 * @param [in]  options        Connect options such as pairing.
 */
void pbdrv_bluetooth_peripheral_scan_and_connect(
    pbio_task_t *task,
    pbdrv_bluetooth_ad_match_t match_adv,
    pbdrv_bluetooth_ad_match_t match_adv_rsp,
    pbdrv_bluetooth_receive_handler_t notification_handler,
    pbdrv_bluetooth_peripheral_options_t options);

/**
 * Gets the name of the connected peripheral.
 *
 * @return  The name of the connected peripheral. May not be set.
 */
const char *pbdrv_bluetooth_peripheral_get_name(void);

/**
 * Find a characteristic by UUID and properties.
 *
 * If found, the value handle in the characteristic is set.
 *
 * @param [in]  task           The task that is used to wait for completion.
 * @param [in]  characteristic The characteristic to discover.
 */
void pbdrv_bluetooth_periperal_discover_characteristic(pbio_task_t *task, pbdrv_bluetooth_peripheral_char_t *characteristic);

/**
 * Read a characteristic.
 *
 * @param [in]  task           The task that is used to wait for completion.
 * @param [in]  characteristic The characteristic to read.
 */
void pbdrv_bluetooth_periperal_read_characteristic(pbio_task_t *task, pbdrv_bluetooth_peripheral_char_t *characteristic);

// TODO: make this a generic write without response function
void pbdrv_bluetooth_peripheral_write(pbio_task_t *task, pbdrv_bluetooth_value_t *value);
// TODO: make this a generic disconnect
void pbdrv_bluetooth_peripheral_disconnect(pbio_task_t *task);

/**
 * Starts broadcasting undirected, non-connectable, non-scannable advertisement
 * data.
 *
 * Call again to update the advertising data if needed.
 *
 * The advertising data must follow the Bluetooth specification. The length
 * is validated, but the data is itself is not.
 *
 * @param [out] task    An uninitialized task to be filled in.
 * @param [in]  value   The advertising data.
 */
void pbdrv_bluetooth_start_broadcasting(pbio_task_t *task, pbdrv_bluetooth_value_t *value);

/**
 * Stops broadcasting that was started by pbdrv_bluetooth_start_broadcasting().
 *
 * It is safe to call this function even when broadcasting has not been started.
 *
 * @param [out] task    An uninitialized task to be filled in.
 */
void pbdrv_bluetooth_stop_broadcasting(pbio_task_t *task);

/**
 * Starts observing, non-connectable, non-scannable advertisements.
 *
 * It is safe to call this function multiple times without stopping first.
 *
 * @param [out] task        An uninitialized task to be filled in.
 * @param [in]  callback    A callback that is called each time advertising data is received.
 */
void pbdrv_bluetooth_start_observing(pbio_task_t *task, pbdrv_bluetooth_start_observing_callback_t callback);

/**
 * Stops observing/scanning that was started by pbdrv_bluetooth_start_observing().
 *
 * It is safe to call this function even when observing has not been started.
 *
 * @param [out] task        An uninitialized task to be filled in.
 */
void pbdrv_bluetooth_stop_observing(pbio_task_t *task);

#else // PBDRV_CONFIG_BLUETOOTH

#define pbdrv_bluetooth_init

static inline void pbdrv_bluetooth_power_on(bool on) {
}

static inline bool pbdrv_bluetooth_is_ready(void) {
    return false;
}

static inline const char *pbdrv_bluetooth_get_hub_name(void) {
    return "Pybricks Hub";
}

static inline const char *pbdrv_bluetooth_get_fw_version(void) {
    return "";
}

static inline void pbdrv_bluetooth_queue_noop(pbio_task_t *task) {
}

static inline void pbdrv_bluetooth_start_advertising(void) {
}

static inline bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    return false;
}

static inline void pbdrv_bluetooth_send(pbdrv_bluetooth_send_context_t *context) {
    context->done();
}

static inline void pbdrv_bluetooth_peripheral_scan_and_connect(
    pbio_task_t *task,
    pbdrv_bluetooth_ad_match_t match_adv,
    pbdrv_bluetooth_ad_match_t match_adv_rsp,
    pbdrv_bluetooth_receive_handler_t notification_handler,
    pbdrv_bluetooth_peripheral_options_t options) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

static inline const char *pbdrv_bluetooth_peripheral_get_name(void) {
    return NULL;
}

static inline void pbdrv_bluetooth_periperal_discover_characteristic(pbio_task_t *task, pbdrv_bluetooth_peripheral_char_t *characteristic) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_periperal_read_characteristic(pbio_task_t *task, pbdrv_bluetooth_peripheral_char_t *characteristic) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_peripheral_write(pbio_task_t *task, pbdrv_bluetooth_value_t *value) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_peripheral_disconnect(pbio_task_t *task) {
}

static inline void pbdrv_bluetooth_start_broadcasting(pbio_task_t *task, pbdrv_bluetooth_value_t *value) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_stop_broadcasting(pbio_task_t *task) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_start_observing(pbio_task_t *task, pbdrv_bluetooth_start_observing_callback_t callback) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbdrv_bluetooth_stop_observing(pbio_task_t *task) {
    task->status = PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @} */
