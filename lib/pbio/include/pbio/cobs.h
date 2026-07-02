// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Consistent Overhead Byte Stuffing (COBS) framing, SPIKE Prime variant.
// https://lego.github.io/spike-prime-docs/encoding.html
//
// Unlike classic COBS, this variant escapes three values (0x00, 0x01 and 0x02)
// so that 0x01 and 0x02 are free to act as message delimiters. A single code
// word therefore encodes both the block size and which of the three delimiter
// values terminated the block:
//
//     code_word = delimiter * MAX_BLOCK_SIZE + block_size + CODE_OFFSET
//
// (0xFF marks a full block not terminated by any delimiter.) After encoding,
// but before delimiting, every byte is XORed with 0x03 so the output over-the
// wire can never contain 0x03 either, which is why 0x00 has to be escaped in
// the first place. The delimiters are not XORed.

#ifndef _PBIO_COBS_H_
#define _PBIO_COBS_H_

#include <stdint.h>

/** Frame delimiter byte (end of a low-priority message). */
#define PBIO_COBS_DELIMITER 0x02

/** High-priority start delimiter. Never emitted; tolerated on decode. */
#define PBIO_COBS_DELIMITER_HIGH_PRIORITY 0x01

/**
 * The encoded payload is XORed with this value so the output never contains
 * 0x03. This works because the payload never contains 0x00 (it is escaped),
 * and XOR with 0x03 maps 0x00 <-> 0x03, so no 0x03 can be produced. The flip
 * side is that a minimum code word of 3 becomes 0x00 on the wire, which is
 * harmless because 0x00 is not a delimiter here (unlike in classic COBS).
 */
#define PBIO_COBS_XOR 0x03

/**
 * The stream escapes the three delimiter values 0x00, 0x01 and 0x02. The
 * largest of these bounds the "is a delimiter" test during encoding.
 */
#define PBIO_COBS_MAX_DELIMITER 0x02

/** Added to a block size to form a code word, keeping it clear of delimiters. */
#define PBIO_COBS_CODE_OFFSET 2

/** Maximum number of bytes represented by a single code word. */
#define PBIO_COBS_MAX_BLOCK_SIZE 84

/** Code word for a full block that is not terminated by a delimiter. */
#define PBIO_COBS_NO_DELIMITER 0xFF

/**
 * Upper bound on the encoded size (including the trailing delimiter) for a
 * decoded message of @p n bytes.
 *
 * The overhead is at most one code word per ::PBIO_COBS_MAX_BLOCK_SIZE bytes
 * (a block hitting the size cap, or an all-delimiter input closing a block per
 * byte), plus the final code word and the trailing delimiter.
 */
#define PBIO_COBS_ENCODED_BUFFER_SIZE(n) ((n) + (n) / PBIO_COBS_MAX_BLOCK_SIZE + 2)

uint32_t pbio_cobs_encode(const uint8_t *src, uint32_t len, uint8_t *dst);

uint32_t pbio_cobs_decode(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t dst_max);

#endif // _PBIO_COBS_H_
