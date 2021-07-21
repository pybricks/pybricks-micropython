// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Nordic UART Service - non-standard but commonly used BLE serial communications
// https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/include/bluetooth/services/nus.html


#include <stdint.h>

/**
 * Nordic UART Service UUID.
 *
 * 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 */
const uint8_t pbio_nus_service_uuid[] = {
    0x6E, 0x40, 0x00, 0x01, 0xB5, 0xA3, 0xF3, 0x93,
    0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E,
};

/**
 * Nordic UART Rx Characteristic UUID.
 *
 * 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
 */
const uint8_t pbio_nus_rx_char_uuid[] = {
    0x6E, 0x40, 0x00, 0x02, 0xB5, 0xA3, 0xF3, 0x93,
    0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E,
};

/**
 * Nordic UART Tx Characteristic UUID.
 *
 * 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
 */
const uint8_t pbio_nus_tx_char_uuid[] = {
    0x6E, 0x40, 0x00, 0x03, 0xB5, 0xA3, 0xF3, 0x93,
    0xE0, 0xA9, 0xE5, 0x0E, 0x24, 0xDC, 0xCA, 0x9E,
};
