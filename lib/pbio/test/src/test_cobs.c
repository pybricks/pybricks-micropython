// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbio/cobs.h>
#include <pbio/util.h>

#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

// Largest decoded message exercised by the round-trip tests. Chosen well above
// PBIO_COBS_MAX_BLOCK_SIZE so that block splitting is covered too.
#define TEST_MAX_DECODED (512)
#define TEST_MAX_ENCODED PBIO_COBS_ENCODED_BUFFER_SIZE(TEST_MAX_DECODED)

/**
 * Encodes @p prefix followed by @p len payload bytes, verifies the framing
 * invariants, then decodes it back and verifies the prefix and payload match.
 * @p len must be at least 1, since a prefix-only frame decodes as 0 (the same
 * value used to report an invalid frame).
 */
static void check_roundtrip(uint8_t prefix, const uint8_t *payload, uint32_t len) {
    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len = pbio_cobs_encode_prefixed(prefix, payload, len, enc);

    // The frame must end with the delimiter and contain no reserved values
    // (0x01, 0x02, 0x03) in its body. 0x00 is allowed in the body.
    tt_want_int_op(enc_len, >=, 2);
    tt_want_int_op(enc[enc_len - 1], ==, PBIO_COBS_DELIMITER);
    for (uint32_t i = 0; i < enc_len - 1; i++) {
        tt_want(enc[i] != 0x01 && enc[i] != 0x02 && enc[i] != 0x03);
    }

    // Encoded size must stay within the advertised bound.
    tt_want_int_op(enc_len, <=, PBIO_COBS_ENCODED_BUFFER_SIZE(len));

    // Decode the frame body (delimiter stripped) and compare.
    uint8_t dec_prefix = 0;
    uint8_t dec[TEST_MAX_DECODED];
    uint32_t dec_len = pbio_cobs_decode_prefixed(enc, enc_len - 1, &dec_prefix, dec, sizeof(dec));
    tt_want_int_op(dec_len, ==, len);
    tt_want_int_op(dec_prefix, ==, prefix);
    tt_want_int_op(memcmp(dec, payload, len), ==, 0);
}

/**
 * A few known-answer vectors to lock the wire format, matching the reference
 * Python implementation (tools/flash/serdev/spike.py).
 */
static void test_cobs_known_vectors(void *env) {
    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len;

    // Prefix 0x00 with no payload: two code words (3, 3) XORed to (0, 0), then
    // delimiter.
    static const uint8_t enc_zero[] = { 0x00, 0x00, 0x02 };
    enc_len = pbio_cobs_encode_prefixed(0x00, NULL, 0, enc);
    tt_want_int_op(enc_len, ==, sizeof(enc_zero));
    tt_want_int_op(memcmp(enc, enc_zero, sizeof(enc_zero)), ==, 0);

    // Prefix 0x41 ("A") followed by "BC": code word 6 plus the three bytes, all
    // XORed, then delimiter.
    static const uint8_t payload_bc[] = { 0x42, 0x43 };
    static const uint8_t enc_abc[] = { 0x05, 0x42, 0x41, 0x40, 0x02 };
    enc_len = pbio_cobs_encode_prefixed(0x41, payload_bc, sizeof(payload_bc), enc);
    tt_want_int_op(enc_len, ==, sizeof(enc_abc));
    tt_want_int_op(memcmp(enc, enc_abc, sizeof(enc_abc)), ==, 0);
}

/**
 * Round-trips a range of structured inputs: single payload bytes, every prefix
 * value, all-delimiter runs, and blocks straddling the 84-byte code-word
 * boundary.
 */
static void test_cobs_edge_cases(void *env) {
    uint8_t payload[TEST_MAX_DECODED] = { 0 };

    // Every possible byte value, once as the sole payload byte and once as the
    // prefix, to exercise both decode paths.
    for (uint32_t b = 0; b < 256; b++) {
        payload[0] = b;
        check_roundtrip(0x99, payload, 1);
        check_roundtrip(b, payload, 1);
    }

    // Runs of only delimiter values (worst case for overhead).
    for (uint8_t v = 0; v <= PBIO_COBS_MAX_DELIMITER; v++) {
        memset(payload, v, sizeof(payload));
        check_roundtrip(v, payload, sizeof(payload));
    }

    // Runs of only ordinary bytes around the block-size boundary.
    memset(payload, 0xAA, sizeof(payload));
    for (uint32_t len = PBIO_COBS_MAX_BLOCK_SIZE - 2; len <= PBIO_COBS_MAX_BLOCK_SIZE + 2; len++) {
        check_roundtrip(0xAA, payload, len);
    }
    check_roundtrip(0xAA, payload, 2 * PBIO_COBS_MAX_BLOCK_SIZE);
    check_roundtrip(0xAA, payload, 2 * PBIO_COBS_MAX_BLOCK_SIZE + 1);
}

/**
 * Simple reproducible PRNG so each parallel iteration is independent and the
 * test is deterministic.
 */
static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

/**
 * Round-trips many random messages. Each iteration is independent, so run them
 * across all CPU cores.
 */
static void test_cobs_roundtrip_random(void *env) {
    #pragma omp parallel for
    for (uint32_t i = 1; i <= 200000; i++) {
        uint32_t state = i;
        uint8_t payload[TEST_MAX_DECODED];

        uint8_t prefix = xorshift32(&state) & 0xFF;

        // At least one payload byte, since a prefix-only frame is reported as 0.
        uint32_t len = 1 + xorshift32(&state) % (TEST_MAX_DECODED - 1);
        for (uint32_t j = 0; j < len; j++) {
            // Bias towards delimiter values so escaping is exercised heavily.
            uint32_t r = xorshift32(&state);
            payload[j] = (r & 3) ? (r % 3) : (r >> 8) & 0xFF;
        }

        check_roundtrip(prefix, payload, len);
    }
}

/**
 * A leading high-priority delimiter (0x01) is not something we emit, but a host
 * might. The decoder must skip it and decode the rest unchanged.
 */
static void test_cobs_high_priority_prefix(void *env) {
    uint8_t prefix = 0x10;
    static const uint8_t payload[] = { 0x00, 0x20, 0x01, 0x02, 0x30 };

    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len = pbio_cobs_encode_prefixed(prefix, payload, sizeof(payload), enc);

    // Build a frame body prefixed with the high-priority start delimiter.
    uint8_t framed[TEST_MAX_ENCODED + 1];
    framed[0] = PBIO_COBS_DELIMITER_HIGH_PRIORITY;
    memcpy(&framed[1], enc, enc_len - 1); // drop trailing delimiter

    uint8_t dec_prefix = 0;
    uint8_t dec[TEST_MAX_DECODED];
    uint32_t dec_len = pbio_cobs_decode_prefixed(framed, enc_len, &dec_prefix, dec, sizeof(dec));
    tt_want_int_op(dec_len, ==, sizeof(payload));
    tt_want_int_op(dec_prefix, ==, prefix);
    tt_want_int_op(memcmp(dec, payload, sizeof(payload)), ==, 0);
}

/**
 * Decoding into a buffer that is too small must fail cleanly (return 0) rather
 * than overflow.
 */
static void test_cobs_decode_overflow(void *env) {
    uint8_t prefix = 0x41;
    static const uint8_t payload[] = { 0x42, 0x43, 0x44, 0x45 };

    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len = pbio_cobs_encode_prefixed(prefix, payload, sizeof(payload), enc);

    uint8_t dec_prefix = 0;
    uint8_t dec[sizeof(payload) - 1];
    uint32_t dec_len = pbio_cobs_decode_prefixed(enc, enc_len - 1, &dec_prefix, dec, sizeof(dec));
    tt_want_int_op(dec_len, ==, 0);
}

/**
 * An empty frame body (or one containing only a high-priority prefix) decodes
 * to nothing, reported as 0.
 */
static void test_cobs_decode_empty(void *env) {
    uint8_t dec_prefix = 0;
    uint8_t dec[TEST_MAX_DECODED];

    tt_want_int_op(pbio_cobs_decode_prefixed(NULL, 0, &dec_prefix, dec, sizeof(dec)), ==, 0);

    static const uint8_t only_prefix[] = { PBIO_COBS_DELIMITER_HIGH_PRIORITY };
    tt_want_int_op(pbio_cobs_decode_prefixed(only_prefix, sizeof(only_prefix), &dec_prefix, dec, sizeof(dec)), ==, 0);
}

struct testcase_t pbio_cobs_tests[] = {
    PBIO_TEST(test_cobs_known_vectors),
    PBIO_TEST(test_cobs_edge_cases),
    PBIO_TEST(test_cobs_roundtrip_random),
    PBIO_TEST(test_cobs_high_priority_prefix),
    PBIO_TEST(test_cobs_decode_overflow),
    PBIO_TEST(test_cobs_decode_empty),
    END_OF_TESTCASES
};
