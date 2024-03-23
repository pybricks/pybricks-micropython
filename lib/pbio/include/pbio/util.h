// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

/**
 * @addtogroup Utility pbio/util: Utility Functions
 *
 * Miscellaneous helper functions.
 *
 * @{
 */
#ifndef _PBIO_UTIL_H_
#define _PBIO_UTIL_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>

/**
 * Converts @p str to a quoted string.
 * @param [in]  str     The text to be quoted.
 */
#define PBIO_STR(str) #str

/**
 * Converts @p str to a quoted string after expanding macros.
 * @param [in]  str     The text to be quoted.
 */
#define PBIO_XSTR(str) PBIO_STR(str)

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

#ifndef DOXYGEN
static inline
#endif
/**
 * Gets 16-bit little endian value from buffer.
 *
 * @param [in]  buf     The buffer.
 * @return              The value.
 */
uint16_t pbio_get_uint16_le(const uint8_t *buf) {
    return buf[0] | (buf[1] << 8);
}

#ifndef DOXYGEN
static inline
#endif
/**
 * Sets 16-bit little endian value in buffer.
 *
 * @param [in]  buf     The buffer.
 * @param [in]  value   The value.
 */
void pbio_set_uint16_le(uint8_t *buf, uint16_t value) {
    buf[0] = value;
    buf[1] = value >> 8;
}

#ifndef DOXYGEN
static inline
#endif
/**
 * Gets 32-bit little endian value from buffer.
 *
 * @param [in]  buf     The buffer.
 * @return              The value.
 */
uint32_t pbio_get_uint32_le(const uint8_t *buf) {
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

#ifndef DOXYGEN
static inline
#endif
/**
 * Packs 32-bit little endian value into buffer.
 *
 * @param [in]  buf     The buffer.
 * @param [in]  value   The value.
 */
void pbio_set_uint32_le(uint8_t *buf, uint32_t value) {
    buf[0] = value;
    buf[1] = value >> 8;
    buf[2] = value >> 16;
    buf[3] = value >> 24;
}

#ifndef DOXYGEN
static inline
#endif
/**
 * Packs 32-bit big endian value into buffer.
 *
 * @param [in]  buf     The buffer.
 * @param [in]  value   The value.
 */
void pbio_set_uint32_be(uint8_t *buf, uint32_t value) {
    buf[0] = value >> 24;
    buf[1] = value >> 16;
    buf[2] = value >> 8;
    buf[3] = value;
}

void pbio_uuid128_le_copy(uint8_t *dst, const uint8_t *src);
bool pbio_uuid128_reverse_compare(const uint8_t *uuid1, const uint8_t *uuid2);
void pbio_uuid128_reverse_copy(uint8_t *dst, const uint8_t *src);

/**
 * Declares a new oneshot state variable.
 * @param [in]  name    The name of the variable.
 */
#define PBIO_ONESHOT(name) bool name = false

bool pbio_oneshot(bool value, bool *state);

#endif // _PBIO_UTIL_H_

/** @} */
