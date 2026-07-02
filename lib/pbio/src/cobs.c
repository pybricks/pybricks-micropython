// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Consistent Overhead Byte Stuffing (COBS) framing, SPIKE Prime variant. See
// pbio/cobs.h for a description of the scheme.

#include <stdbool.h>
#include <stdint.h>

#include <pbio/cobs.h>

/**
 * COBS-encodes @p len bytes from @p src into @p dst, XORs the result with
 * ::PBIO_COBS_XOR and appends the frame delimiter. @p dst must have room for
 * at least ::PBIO_COBS_ENCODED_BUFFER_SIZE(@p len) bytes.
 *
 * While the decoder tolerates a leading high-priority start delimiter (0x01),
 * this encoder never emits one. All messages are treated as low priority.
 *
 * @param [in]  src     Data to encode.
 * @param [in]  len     Number of bytes to encode.
 * @param [out] dst     Buffer to write the framed output to.
 * @return              Number of bytes written, including the trailing delimiter.
 */
uint32_t pbio_cobs_encode(const uint8_t *src, uint32_t len, uint8_t *dst) {
    uint32_t write_idx = 0;

    // Start a block by reserving a code word slot, provisionally marking it as
    // a full block with no trailing delimiter.
    uint32_t code_idx = write_idx++;
    dst[code_idx] = PBIO_COBS_NO_DELIMITER;
    uint8_t block = 1;

    for (uint32_t read_idx = 0; read_idx < len; read_idx++) {
        uint8_t byte = src[read_idx];

        if (byte > PBIO_COBS_MAX_DELIMITER) {
            // Ordinary byte: copy it and grow the current block.
            dst[write_idx++] = byte;
            block++;
        }

        if (byte <= PBIO_COBS_MAX_DELIMITER || block > PBIO_COBS_MAX_BLOCK_SIZE) {
            if (byte <= PBIO_COBS_MAX_DELIMITER) {
                // Block ended on a delimiter value: fold both the delimiter and
                // the block size into the code word. A block ended purely by
                // the size limit keeps its 0xFF (no-delimiter) code word.
                dst[code_idx] = byte * PBIO_COBS_MAX_BLOCK_SIZE + block + PBIO_COBS_CODE_OFFSET;
            }
            // Begin a new block.
            code_idx = write_idx++;
            dst[code_idx] = PBIO_COBS_NO_DELIMITER;
            block = 1;
        }
    }

    // Finalize the last (open) block.
    dst[code_idx] = block + PBIO_COBS_CODE_OFFSET;

    // XOR so the encoded body never contains 0x03.
    for (uint32_t i = 0; i < write_idx; i++) {
        dst[i] ^= PBIO_COBS_XOR;
    }

    // Frame the message. We only ever send low priority, so this is just the
    // trailing end delimiter with no high-priority prefix.
    dst[write_idx++] = PBIO_COBS_DELIMITER;

    return write_idx;
}

/**
 * Decodes a single code word into the delimiter value it carries (if any) and
 * the number of bytes in its block (including the code word itself).
 *
 * @param [in]  code       The code word.
 * @param [out] has_value  Whether the block is terminated by a delimiter value.
 * @param [out] value      The delimiter value, valid only if @p has_value.
 * @param [out] block      Number of bytes in the block, including the code word.
 */
static void pbio_cobs_unescape(uint8_t code, bool *has_value, uint8_t *value, uint8_t *block) {
    if (code == PBIO_COBS_NO_DELIMITER) {
        *has_value = false;
        *block = PBIO_COBS_MAX_BLOCK_SIZE + 1;
        return;
    }

    uint8_t offset = code - PBIO_COBS_CODE_OFFSET;
    uint8_t v = offset / PBIO_COBS_MAX_BLOCK_SIZE;
    uint8_t b = offset % PBIO_COBS_MAX_BLOCK_SIZE;
    if (b == 0) {
        // A full block that still ends in a delimiter borrows from the value.
        b = PBIO_COBS_MAX_BLOCK_SIZE;
        v -= 1;
    }

    *has_value = true;
    *value = v;
    *block = b;
}

/**
 * Reverses the XOR and COBS-decodes @p len bytes from @p src (a single frame
 * with the trailing delimiter already stripped) into @p dst.
 *
 * @param [in]  src     Frame body to decode (delimiter already stripped).
 * @param [in]  len     Number of bytes in @p src.
 * @param [out] dst     Buffer to write the decoded message to.
 * @param [in]  dst_max Capacity of @p dst.
 * @return              Number of decoded bytes, or 0 if the frame was empty or
 *                      malformed (including overflowing @p dst).
 */
uint32_t pbio_cobs_decode(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t dst_max) {
    uint32_t read_idx = 0;

    // Skip an optional high-priority start delimiter. We never emit one, but a
    // host might; we treat every incoming message as low priority regardless.
    // This byte is part of the framing and is not XORed.
    if (len > 0 && src[read_idx] == PBIO_COBS_DELIMITER_HIGH_PRIORITY) {
        read_idx++;
    }

    if (read_idx >= len) {
        return 0;
    }

    uint32_t write_idx = 0;

    bool has_value;
    uint8_t value;
    uint8_t block;
    pbio_cobs_unescape(src[read_idx++] ^ PBIO_COBS_XOR, &has_value, &value, &block);

    while (read_idx < len) {
        uint8_t byte = src[read_idx++] ^ PBIO_COBS_XOR;

        if (--block > 0) {
            // Still inside the current block: emit the byte verbatim.
            if (write_idx >= dst_max) {
                return 0;
            }
            dst[write_idx++] = byte;
            continue;
        }

        // Block completed. Emit the delimiter value it encoded, if any, then
        // interpret this byte as the next code word.
        if (has_value) {
            if (write_idx >= dst_max) {
                return 0;
            }
            dst[write_idx++] = value;
        }
        pbio_cobs_unescape(byte, &has_value, &value, &block);
    }

    return write_idx;
}
