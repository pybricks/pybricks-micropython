// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>

/**
 * Copmpares two 128-bit UUIDs with opposite byte ordering for equality.
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
