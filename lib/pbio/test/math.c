
#include <stdio.h>

#include <pbio/math.h>

#include <tinytest.h>
#include <tinytest_macros.h>

void test_sqrt(void *env) {
    tt_want(pbio_math_sqrt(0) == 0);
    tt_want(pbio_math_sqrt(1) == 1);
    tt_want(pbio_math_sqrt(4) == 2);
    tt_want(pbio_math_sqrt(400) == 20);
    tt_want(pbio_math_sqrt(40000) == 200);
    tt_want(pbio_math_sqrt(400000000) == 20000);
}

void test_mul_i32_fix16(void *env) {
    // fix16_maximum == 32767.99998474121
    // fix16_minimum == -32768.0
    tt_want_int_op(pbio_math_mul_i32_fix16(0, F16(0.0)), ==, 0);
    tt_want_int_op(pbio_math_mul_i32_fix16(1, fix16_maximum), ==, 32768);
    tt_want_int_op(pbio_math_mul_i32_fix16(1, fix16_minimum), ==, -32768);
    tt_want_int_op(pbio_math_mul_i32_fix16(2, fix16_maximum), ==, 65536);
    tt_want_int_op(pbio_math_mul_i32_fix16(2, fix16_minimum), ==, -65536);
    tt_want_int_op(pbio_math_mul_i32_fix16(32768, fix16_maximum), ==, 1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(32768, fix16_minimum), ==, -1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(65535, fix16_maximum), ==, 2147450879);
    tt_want_int_op(pbio_math_mul_i32_fix16(65535, fix16_minimum), ==, -2147450880);
    // we can safely multiply up to 2^16 with arbitrary fix16 value.
    tt_want_int_op(pbio_math_mul_i32_fix16(65536, fix16_maximum), ==, INT32_MAX);
    tt_want_int_op(pbio_math_mul_i32_fix16(65536, fix16_minimum), ==, INT32_MIN);
    tt_want_int_op(pbio_math_mul_i32_fix16(-65536, fix16_maximum), ==, INT32_MIN + 1);
    tt_want_int_op(pbio_math_mul_i32_fix16(-65536, fix16_minimum), ==, INT32_MIN); // overflow!
    // we can safely multiply up to +/-INT32_MAX with fix16 values less than or equal to one.
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MAX, F16(0.5)), ==, 1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MIN, F16(0.5)), ==, -1073741824);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MAX, F16(1.0)), ==, INT32_MAX);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MIN, F16(1.0)), ==, INT32_MIN);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MAX, F16(-1.0)), ==, INT32_MIN + 1);
    tt_want_int_op(pbio_math_mul_i32_fix16(-INT32_MAX, F16(-1.0)), ==, INT32_MAX);
    tt_want_int_op(pbio_math_mul_i32_fix16(INT32_MIN, F16(-1.0)), ==, INT32_MIN); // overflow!
}
