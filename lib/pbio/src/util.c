// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * Compares a 128-bit UUID from @p le in little endian format with
 * the given uuid @p uuid.
 *
 * According to RFC 4122, the UUID is grouped into the following:
 * 1) One 32-bit
 * 2) Two 16-bit
 * 3) Eight 8-bit
 *
 * @param [in]  le      The little endian UUID.
 * @param [in]  uuid    The UUID to compare against.
 */
bool pbio_uuid128_le_compare(const uint8_t *le, const uint8_t *uuid) {
    return (le[0] == uuid[3]) &&
           (le[1] == uuid[2]) &&
           (le[2] == uuid[1]) &&
           (le[3] == uuid[0]) &&
           (le[4] == uuid[5]) &&
           (le[5] == uuid[4]) &&
           (le[6] == uuid[7]) &&
           (le[7] == uuid[6]) &&
           (memcmp(&le[8], &uuid[8], 8) == 0);
}

/**
 * Copies a 128-bit UUID from @p src to a buffer @p dst,
 * which is a buffer used by a little endian medium.
 *
 * According to RFC 4122, the UUID is grouped into the following:
 * 1) One 32-bit
 * 2) Two 16-bit
 * 3) Eight 8-bit
 *
 * @param [in]  dst     The destination array.
 * @param [in]  src     The UUID to reverse and copy.
 */
void pbio_uuid128_le_copy(uint8_t *dst, const uint8_t *src) {
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];

    dst[4] = src[5];
    dst[5] = src[4];

    dst[6] = src[7];
    dst[7] = src[6];

    memcpy(&dst[8], &src[8], 8);
}

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
