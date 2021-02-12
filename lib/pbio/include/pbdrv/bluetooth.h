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

/** Flags indicating which services are connected. */
typedef enum {
    /** The Pybricks service is connected (notifications enabled). */
    PBDRV_BLUETOOTH_CONNECTION_PYBRICKS = 1 << 0,
    /** The nRF UART service is connected (notifications enabled). */
    PBDRV_BLUETOOTH_CONNECTION_UART = 1 << 1,
    /** At least one service is connected. */
    PBDRV_BLUETOOTH_CONNECTION_ANY = ~0,
} pbdrv_bluetooth_connection_t;

/**
 * Callback that is called when a Bluetooth event occurs.
 */
typedef void (*pbdrv_bluetooth_on_event_t)(void);

/**
 * Callback that is called when Tx is done.
 */
typedef void (*pbdrv_bluetooth_tx_done_t)(void);

/**
 * Callback that is called when Rx data is received.
 *
 * @param [in]  data        The data that was received.
 * @param [in]  size        The size of @p data in bytes.
 */
typedef void (*pbdrv_bluetooth_on_rx_t)(const uint8_t *data, uint8_t size);

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
 * @param [in]  connection  The type of connection of interest or
 *                          ::PBDRV_BLUETOOTH_CONNECTION_ANY to test if there
 *                          is any connection all.
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
 * Requests for @p data to be sent via the Pybricks characteristic.
 *
 * It is up to the caller to verify that the Pybricks service is connected and
 * that any previous Tx request is done before calling this function.
 *
 * @param [in]  data        The data buffer to send. Must remain valid until
 *                          @p done is called.
 * @param [in]  size        The size of @p data in bytes.
 * @param [in]  done        A function that will be called when the data has
 *                          been sent.
 */
void pbdrv_bluetooth_pybricks_tx(const uint8_t *data, uint8_t size, pbdrv_bluetooth_tx_done_t done);

/**
 * Registers a callback that will be called when data is received on the
 * Pybricks characteristic.
 *
 * @param [in]  on_rx       The function that will be called.
 */
void pbdrv_bluetooth_pybricks_set_on_rx(pbdrv_bluetooth_on_rx_t on_rx);

/**
 * Requests for @p data to be sent via the nRF UART Tx characteristic.
 *
 * It is up to the caller to verify that the nRF UART service is connected and
 * that any previous Tx request is done before calling this function.
 *
 * @param [in]  data        The data buffer to send. Must remain valid until
 *                          @p done is called.
 * @param [in]  size        The size of @p data in bytes.
 * @param [in]  done        A function that will be called when the data has
 *                          been sent.
 */
void pbdrv_bluetooth_uart_tx(const uint8_t *data, uint8_t size, pbdrv_bluetooth_tx_done_t done);

/**
 * Registers a callback that will be called when data is received on the nRF
 * UART Rx characteristic.
 *
 * @param [in]  on_rx       The function that will be called.
 */
void pbdrv_bluetooth_uart_set_on_rx(pbdrv_bluetooth_on_rx_t on_rx);

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

static inline void pbdrv_bluetooth_pybricks_tx(const uint8_t *data, uint8_t size, pbdrv_bluetooth_tx_done_t done) {
}

static inline void pbdrv_bluetooth_pybricks_set_on_rx(pbdrv_bluetooth_on_rx_t on_rx) {
}

static inline void pbdrv_bluetooth_uart_tx(const uint8_t *data, uint8_t size, pbdrv_bluetooth_tx_done_t done) {
}

static inline void pbdrv_bluetooth_uart_set_on_rx(pbdrv_bluetooth_on_rx_t on_rx) {
}

#endif // PBDRV_CONFIG_BLUETOOTH

#endif // _PBDRV_BLUETOOTH_H_

/** @} */
