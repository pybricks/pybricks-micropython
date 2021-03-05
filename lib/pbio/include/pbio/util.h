// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _PBIO_UTIL_H_
#define _PBIO_UTIL_H_

#include <stdint.h>
#include <sys/cdefs.h>

/**
 * Get the length of a fixed-sized array.
 * @param a     The array
 */
#define PBIO_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/**
 * Get a pointer to the structure that contains another structure.
 * @param ptr       Pointer to the child structure
 * @param type      The type of the parent structure
 * @param member    The name of the field containing ptr
 */
#ifdef __containerof
#define PBIO_CONTAINER_OF __containerof
#else
#include <stddef.h>
#define PBIO_CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

/**
 * Gets 16-bit little endian value from buffer.
 *
 * @param [in]  buf     The buffer.
 * @return              The value.
 */
static inline uint16_t pbio_get_uint16_le(const uint8_t *buf) {
    return buf[0] | (buf[1] << 8);
}

/**
 * Sets 16-bit little endian value in buffer.
 *
 * @param [in]  buf     The buffer.
 * @param [in]  value   The value.
 */
static inline void pbio_set_uint16_le(uint8_t *buf, uint16_t value) {
    buf[0] = value;
    buf[1] = value >> 8;
}

/**
 * Gets 32-bit little endian value from buffer.
 *
 * @param [in]  buf     The buffer.
 * @return              The value.
 */
static inline uint32_t pbio_get_uint32_le(const uint8_t *buf) {
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

/**
 * Packs 32-bit little endian value into buffer.
 *
 * @param [in]  buf     The buffer.
 * @param [in]  value   The value.
 */
static inline void pbio_set_uint32_le(uint8_t *buf, uint32_t value) {
    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
}

#endif // _PBIO_UTIL_H_
