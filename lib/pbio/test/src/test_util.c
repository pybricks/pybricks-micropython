// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdio.h>
#include <string.h>

#include <pbio/util.h>
#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

static const uint8_t test_uuid[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};

static const uint8_t test_diff_at_start_uuid[] = {
    0x0F, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};

static const uint8_t test_diff_at_end_uuid[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x00,
};

static const uint8_t test_reversed_uuid[] = {
    0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
    0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
};

static void test_uuid128_reverse_compare(void *env) {
    tt_want(pbio_uuid128_reverse_compare(test_reversed_uuid, test_uuid));
    tt_want(!pbio_uuid128_reverse_compare(test_reversed_uuid, test_diff_at_start_uuid));
    tt_want(!pbio_uuid128_reverse_compare(test_reversed_uuid, test_diff_at_end_uuid));
    tt_want(!pbio_uuid128_reverse_compare(test_reversed_uuid, test_reversed_uuid));
}

static void test_uuid128_reverse_copy(void *env) {
    uint8_t uuid[16];

    pbio_uuid128_reverse_copy(uuid, test_uuid);
    tt_want_int_op(memcmp(uuid, test_reversed_uuid, 16), ==, 0);
}

static void test_oneshot(void *env) {
    PBIO_ONESHOT(test_oneshot);

    // multiple calls with false value don't trigger oneshot
    tt_want(!pbio_oneshot(false, &test_oneshot));
    tt_want(!pbio_oneshot(false, &test_oneshot));

    // transition from false to true triggers oneshot only once
    tt_want(pbio_oneshot(true, &test_oneshot));
    tt_want(!pbio_oneshot(true, &test_oneshot));
    tt_want(!pbio_oneshot(true, &test_oneshot));

    // transition from true to false does not trigger oneshot
    tt_want(!pbio_oneshot(false, &test_oneshot));

    // transition from false to true resets oneshot
    tt_want(pbio_oneshot(true, &test_oneshot));
}

struct testcase_t pbio_util_tests[] = {
    PBIO_TEST(test_uuid128_reverse_compare),
    PBIO_TEST(test_uuid128_reverse_copy),
    PBIO_TEST(test_oneshot),
    END_OF_TESTCASES
};
