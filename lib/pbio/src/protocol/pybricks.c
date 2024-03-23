// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

// Pybricks communication protocol

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/protocol.h>
#include <pbio/util.h>

_Static_assert(NUM_PBIO_PYBRICKS_STATUS <= sizeof(uint32_t) * 8,
    "oh no, we added too many status flags");

/**
 * Writes Pybricks status report command to @p buf
 *
 * @param [in]  buf     The buffer to hold the binary data.
 * @param [in]  flags   The status flags.
 * @return              The number of bytes written to @p buf.
 */
uint32_t pbio_pybricks_event_status_report(uint8_t *buf, uint32_t flags) {
    buf[0] = PBIO_PYBRICKS_EVENT_STATUS_REPORT;
    pbio_set_uint32_le(&buf[1], flags);
    return 5;
}

/**
 * Encodes the value of the Pybricks hub capabilities characteristic.
 *
 * @param [in]  buf                 A buffer where the result will be written.
 *                                  Must be at least ::PBIO_PYBRICKS_HUB_CAPABILITIES_VALUE_SIZE bytes.
 * @param [in]  max_char_size       The maximum characteristic value size (negotiated MTU - 3).
 * @param [in]  feature_flags       The feature flags.
 * @param [in]  max_user_prog_size  The maximum allowable size for the user program.
 */
void pbio_pybricks_hub_capabilities(uint8_t *buf,
    uint16_t max_char_size,
    pbio_pybricks_feature_flags_t feature_flags,
    uint32_t max_user_prog_size) {

    pbio_set_uint16_le(&buf[0], max_char_size);
    pbio_set_uint32_le(&buf[2], feature_flags);
    pbio_set_uint32_le(&buf[6], max_user_prog_size);
}

/**
 * Pybricks Service UUID.
 *
 * C5F50001-8280-46DA-89F4-6D8051E4AEEF
 *
 * @since Protocol v1.0.0
 */
const uint8_t pbio_pybricks_service_uuid[] = {
    0xC5, 0xF5, 0x00, 0x01, 0x82, 0x80, 0x46, 0xDA,
    0x89, 0xF4, 0x6D, 0x80, 0x51, 0xE4, 0xAE, 0xEF,
};

/**
 * Pybricks Command/Event Characteristic UUID.
 *
 * C5F50002-8280-46DA-89F4-6D8051E4AEEF
 *
 * @since Protocol v1.0.0
 */
const uint8_t pbio_pybricks_command_event_char_uuid[] = {
    0xC5, 0xF5, 0x00, 0x02, 0x82, 0x80, 0x46, 0xDA,
    0x89, 0xF4, 0x6D, 0x80, 0x51, 0xE4, 0xAE, 0xEF,
};

/**
 * Pybricks Hub Capabilities Characteristic UUID.
 *
 * C5F50003-8280-46DA-89F4-6D8051E4AEEF
 *
 * @since Protocol v1.2.0
 */
const uint8_t pbio_pybricks_hub_capabilities_char_uuid[] = {
    0xC5, 0xF5, 0x00, 0x03, 0x82, 0x80, 0x46, 0xDA,
    0x89, 0xF4, 0x6D, 0x80, 0x51, 0xE4, 0xAE, 0xEF,
};

// Standard BLE UUIDs used as part of Pybricks "protocol".

/** Bluetooth Device Information Service UUID. */
const uint16_t pbio_gatt_device_info_service_uuid = 0x180A;

/** Bluetooth Device Name Characteristic UUID. */
const uint16_t pbio_gatt_device_name_char_uuid = 0x2A00;

/** Bluetooth Firmware Version Characteristic UUID. */
const uint16_t pbio_gatt_firmware_version_char_uuid = 0x2A26;

/** Bluetooth Software Version Characteristic UUID (Pybricks protocol version). */
const uint16_t pbio_gatt_software_version_char_uuid = 0x2A28;

/** Bluetooth PnP ID Characteristic UUID. */
const uint16_t pbio_gatt_pnp_id_char_uuid = 0x2A50;

/**
 * Converts a ::pbio_error_t to a ::pbio_pybricks_error_t for commands.
 *
 * @param [in]  error   The ::pbio_error_t.
 * @returns             The equivelent ::pbio_pybricks_error_t.
 */
pbio_pybricks_error_t pbio_pybricks_error_from_pbio_error(pbio_error_t error) {
    switch (error) {
        case PBIO_SUCCESS:
            return PBIO_PYBRICKS_ERROR_OK;
        case PBIO_ERROR_INVALID_ARG:
            return PBIO_PYBRICKS_ERROR_VALUE_NOT_ALLOWED;
        case PBIO_ERROR_BUSY:
            return PBIO_PYBRICKS_ERROR_BUSY;
        case PBIO_ERROR_NOT_SUPPORTED:
            return PBIO_PYBRICKS_ERROR_INVALID_COMMAND;
        default:
            // to keep code size down, only know used values are included
            // in the map and this is a fallback in case we missed something
            return PBIO_PYBRICKS_ERROR_UNLIKELY_ERROR;
    }
}
