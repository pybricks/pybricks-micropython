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
 * Encodes @p src, verifies the framing invariants, then decodes it back and
 * verifies the result matches the original.
 */
static void check_roundtrip(const uint8_t *src, uint32_t len) {
    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len = pbio_cobs_encode(src, len, enc);

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
    uint8_t dec[TEST_MAX_DECODED];
    uint32_t dec_len = pbio_cobs_decode(enc, enc_len - 1, dec, sizeof(dec));
    tt_want_int_op(dec_len, ==, len);
    tt_want_int_op(memcmp(dec, src, len), ==, 0);
}

/**
 * A few known-answer vectors to lock the wire format, matching the reference
 * Python implementation (tools/flash/serdev/spike.py).
 */
static void test_cobs_known_vectors(void *env) {
    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len;

    // Empty message: single code word (3) XORed to 0x00, then delimiter.
    static const uint8_t empty[] = { 0x00, 0x02 };
    enc_len = pbio_cobs_encode((const uint8_t *)"", 0, enc);
    tt_want_int_op(enc_len, ==, sizeof(empty));
    tt_want_int_op(memcmp(enc, empty, sizeof(empty)), ==, 0);

    // Single zero byte: two code words (3, 3) XORed to (0, 0), then delimiter.
    static const uint8_t src_zero[] = { 0x00 };
    static const uint8_t enc_zero[] = { 0x00, 0x00, 0x02 };
    enc_len = pbio_cobs_encode(src_zero, sizeof(src_zero), enc);
    tt_want_int_op(enc_len, ==, sizeof(enc_zero));
    tt_want_int_op(memcmp(enc, enc_zero, sizeof(enc_zero)), ==, 0);

    // "ABC": code word 6 plus the three bytes, all XORed, then delimiter.
    static const uint8_t src_abc[] = { 0x41, 0x42, 0x43 };
    static const uint8_t enc_abc[] = { 0x05, 0x42, 0x41, 0x40, 0x02 };
    enc_len = pbio_cobs_encode(src_abc, sizeof(src_abc), enc);
    tt_want_int_op(enc_len, ==, sizeof(enc_abc));
    tt_want_int_op(memcmp(enc, enc_abc, sizeof(enc_abc)), ==, 0);
}

/**
 * Round-trips a range of structured inputs: empty, single bytes, all-delimiter
 * runs, and blocks straddling the 84-byte code-word boundary.
 */
static void test_cobs_edge_cases(void *env) {
    uint8_t src[TEST_MAX_DECODED] = { 0 };

    // Empty.
    check_roundtrip(src, 0);

    // Each possible single byte value.
    for (uint32_t b = 0; b < 256; b++) {
        src[0] = b;
        check_roundtrip(src, 1);
    }

    // Runs of only delimiter values (worst case for overhead).
    for (uint8_t v = 0; v <= PBIO_COBS_MAX_DELIMITER; v++) {
        memset(src, v, sizeof(src));
        check_roundtrip(src, sizeof(src));
    }

    // Runs of only ordinary bytes around the block-size boundary.
    memset(src, 0xAA, sizeof(src));
    for (uint32_t len = PBIO_COBS_MAX_BLOCK_SIZE - 2; len <= PBIO_COBS_MAX_BLOCK_SIZE + 2; len++) {
        check_roundtrip(src, len);
    }
    check_roundtrip(src, 2 * PBIO_COBS_MAX_BLOCK_SIZE);
    check_roundtrip(src, 2 * PBIO_COBS_MAX_BLOCK_SIZE + 1);
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
        uint8_t src[TEST_MAX_DECODED];

        uint32_t len = xorshift32(&state) % (TEST_MAX_DECODED + 1);
        for (uint32_t j = 0; j < len; j++) {
            // Bias towards delimiter values so escaping is exercised heavily.
            uint32_t r = xorshift32(&state);
            src[j] = (r & 3) ? (r % 3) : (r >> 8) & 0xFF;
        }

        check_roundtrip(src, len);
    }
}

/**
 * A leading high-priority delimiter (0x01) is not something we emit, but a host
 * might. The decoder must skip it and decode the rest unchanged.
 */
static void test_cobs_high_priority_prefix(void *env) {
    static const uint8_t msg[] = { 0x10, 0x00, 0x20, 0x01, 0x02, 0x30 };

    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len = pbio_cobs_encode(msg, sizeof(msg), enc);

    // Build a frame body prefixed with the high-priority start delimiter.
    uint8_t prefixed[TEST_MAX_ENCODED + 1];
    prefixed[0] = PBIO_COBS_DELIMITER_HIGH_PRIORITY;
    memcpy(&prefixed[1], enc, enc_len - 1); // drop trailing delimiter

    uint8_t dec[TEST_MAX_DECODED];
    uint32_t dec_len = pbio_cobs_decode(prefixed, enc_len, dec, sizeof(dec));
    tt_want_int_op(dec_len, ==, sizeof(msg));
    tt_want_int_op(memcmp(dec, msg, sizeof(msg)), ==, 0);
}

/**
 * Decoding into a buffer that is too small must fail cleanly (return 0) rather
 * than overflow.
 */
static void test_cobs_decode_overflow(void *env) {
    static const uint8_t msg[] = { 0x41, 0x42, 0x43, 0x44, 0x45 };

    uint8_t enc[TEST_MAX_ENCODED];
    uint32_t enc_len = pbio_cobs_encode(msg, sizeof(msg), enc);

    uint8_t dec[sizeof(msg) - 1];
    uint32_t dec_len = pbio_cobs_decode(enc, enc_len - 1, dec, sizeof(dec));
    tt_want_int_op(dec_len, ==, 0);
}

/**
 * An empty frame body (or one containing only a high-priority prefix) decodes
 * to nothing.
 */
static void test_cobs_decode_empty(void *env) {
    uint8_t dec[TEST_MAX_DECODED];

    tt_want_int_op(pbio_cobs_decode(NULL, 0, dec, sizeof(dec)), ==, 0);

    static const uint8_t only_prefix[] = { PBIO_COBS_DELIMITER_HIGH_PRIORITY };
    tt_want_int_op(pbio_cobs_decode(only_prefix, sizeof(only_prefix), dec, sizeof(dec)), ==, 0);
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
