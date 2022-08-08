// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>

#include <pbio/util.h>

/**
 * Compares two 128-bit UUIDs with opposite byte ordering for equality.
 *
 * @param [in]  uuid1   A 128-bit UUID in little endian order.
 * @param [in]  uuid2   A 128-bit UUID in little big order.
 * @return              True if the UUIDs are the same, otherwise false.
 */
bool pbio_uuid128_reverse_compare(const uint8_t *uuid1, const uint8_t *uuid2) {
    for (int i = 0; i < 16; i++) {
        if (uuid1[i] != uuid2[15 - i]) {
            return false;
        }
    }

    return true;
}

/**
 * Copies a 128-bit UUID from @p src to @p dst while reversing the byte order.
 *
 * @param [in]  dst     The destination array.
 * @param [in]  src     The UUID to reverse and copy.
 */
void pbio_uuid128_reverse_copy(uint8_t *dst, const uint8_t *src) {
    for (int i = 0; i < 16; i++) {
        dst[i] = src[15 - i];
    }
}

/**
 * Performs a rising-edge oneshot test.
 * @param [in]  value   The value being tested.
 * @param [in]  state   The current internal state of the oneshot.
 * @return              True if the value transitioned from false to true since
 *                      the last call, otherwise false.
 */
bool pbio_oneshot(bool value, bool *state) {
    bool ret = false;

    if (value && !*state) {
        ret = true;
    }

    *state = value;

    return ret;
}

/**
 * Gets checksum for the data in the buffer.
 *
 * @param [in]  buf     The buffer.
 * @return              The value.
 */
uint32_t pbio_util_get_checksum(pbio_util_checksum_type_t type, const uint8_t *buf, uint32_t len) {

    uint32_t checksum = 0;
    switch (type) {
        case PBIO_UTIL_CHECKSUM_TYPE_XOR8_START_FF:
            // Set start value, and then ...
            checksum = 0xFF;
        // ... fallthrough
        case PBIO_UTIL_CHECKSUM_TYPE_XOR8_START_00:
            for (uint32_t i = 0; i < len; i++) {
                checksum ^= buf[i];
            }
            return checksum;
        default:
            return 0;
    }
}
