// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

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

/**
 * BLE characteristic connection identifiers.
 */
typedef enum {
    /* An HCI device connection. */
    PBDRV_BLUETOOTH_CONNECTION_HCI,
    /** The Pybricks service. */
    PBDRV_BLUETOOTH_CONNECTION_PYBRICKS,
    /** The nRF UART service. */
    PBDRV_BLUETOOTH_CONNECTION_UART,
} pbdrv_bluetooth_connection_t;

/** Data structure that holds context needed for BLE notifications. */
typedef struct _pbdrv_bluetooth_tx_context_t pbdrv_bluetooth_tx_context_t;

/**
 * Callback that is called when a Bluetooth event occurs.
 */
typedef void (*pbdrv_bluetooth_on_event_t)(void);

/**
 * Callback that is called when Tx is done.
 * @param context  [in]     The context that was given to pbdrv_bluetooth_tx().
 */
typedef void (*pbdrv_bluetooth_tx_done_t)(pbdrv_bluetooth_tx_context_t *context);

/**
 * Callback that is called when Rx data is received.
 *
 * @param [in]  connection  The characteristic that was written.
 * @param [in]  data        The data that was received.
 * @param [in]  size        The size of @p data in bytes.
 */
typedef void (*pbdrv_bluetooth_handle_rx_t)(pbdrv_bluetooth_connection_t connection, const uint8_t *data, uint8_t size);

struct _pbdrv_bluetooth_tx_context_t {
    /** Callback that is called when the data has been sent. */
    pbdrv_bluetooth_tx_done_t done;
    /** The data to be sent. This data must remain valied until @p done is called. */
    const uint8_t *data;
    /** The size of @p data. */
    uint8_t size;
    /** The connection to use. Only characteristics with notify capability are allowed. */
    pbdrv_bluetooth_connection_t connection;
};

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
 * Starts the advertising process, including configuring advertisements and
 * telling the Bluetooth chip to start advertising. Advertising should
 * automatically stop when a connection is made.
 */
void pbdrv_bluetooth_start_advertising(void);

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
 * that any previous Tx request is done before calling this function.
 *
 * @param [in]  context     The data to be sent and where to send it.
 */
void pbdrv_bluetooth_tx(pbdrv_bluetooth_tx_context_t *context);

/**
 * Registers a callback that will be called when data is received via a
 * characteristic write.
 *
 * @param [in]  handle_rx   The function that will be called.
 */
void pbdrv_bluetooth_set_handle_rx(pbdrv_bluetooth_handle_rx_t handle_rx);

#else // PBDRV_CONFIG_BLUETOOTH

#define pbdrv_bluetooth_init

static inline void pbdrv_bluetooth_power_on(bool on) {
}

static inline bool pbdrv_bluetooth_is_ready(void) {
    return false;
}

static inline void pbdrv_bluetooth_start_advertising(void) {
}

static inline bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    return false;
}

static inline void pbdrv_bluetooth_tx(pbdrv_bluetooth_tx_context_t *context) {
}

static inline void pbdrv_bluetooth_set_handle_rx(pbdrv_bluetooth_handle_rx_t handle_rx) {
}

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @} */
