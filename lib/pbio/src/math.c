// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#include <pbio/math.h>
#include <pbio/util.h>

/**
 * Gets the absolute value.
 *
 * @param [in]  value   The value.
 * @return              The absolute (positive) value.
 */
int32_t pbio_math_abs(int32_t value) {
    return __builtin_abs(value);
}

/**
 * Gets the maximum of two values.
 *
 * @param [in]  a       Value.
 * @param [in]  b       Value.
 * @return              a if it is greater than b, else b.
 */
int32_t pbio_math_max(int32_t a, int32_t b) {
    if (a > b) {
        return a;
    }
    return b;
}

/**
 * Gets the minimum of two values.
 *
 * @param [in]  a       Value.
 * @param [in]  b       Value.
 * @return              a if it is less than b, else b.
 */
int32_t pbio_math_min(int32_t a, int32_t b) {
    if (a < b) {
        return a;
    }
    return b;
}

/**
 * Get the sign of @p a.
 *
 * @param [in]  a   A signed integer value.
 * @return          1 if @p a is positive, -1 if @p a is negative or 0 if @p a
 *                  is 0.
 */
int32_t pbio_math_sign(int32_t a) {
    if (a == 0) {
        return 0;
    }
    return a > 0 ? 1 : -1;
}

/**
 * Checks that the signs of @p a and @p b are not opposite.
 *
 * @param [in]  a   A signed integer value.
 * @param [in]  b   A signed integer value.
 * @return          True if either value is zero or if the signs are the same,
 *                  else false.
 */
bool pbio_math_sign_not_opposite(int32_t a, int32_t b) {
    if (a == 0 || b == 0) {
        return true;
    }
    return (a > 0) == (b > 0);
}

/**
 * Binds a value between a lower and upper limit.
 *
 * If @p value is greater than @p max, then @p max is
 * returned. If @p value is less than @p min, then
 * @p min is returned. Otherwise @p value is returned.
 *
 * @param [in]  value   The value to bind.
 * @param [in]  min     The lower boundary value.
 * @param [in]  max     The upper boundary value.
 * @return              The bounded value.
 */
int32_t pbio_math_bind(int32_t value, int32_t min, int32_t max) {
    assert(max >= min);

    if (value > max) {
        return max;
    }
    if (value < min) {
        return min;
    }
    return value;
}

/**
 * Clamps a value to a +/- limit.
 *
 * If @p value is greater than positive @p abs_max, then positive @p abs_max is
 * returned. If @p value is less than negative @p abs_max, then negative
 * @p abs_max is returned. Otherwise @p value is returned.
 *
 * @param [in]  value   The value to clamp.
 * @param [in]  abs_max The clamp limit. This must be a positive value.
 * @return              The clamped value.
 */
int32_t pbio_math_clamp(int32_t value, int32_t abs_max) {
    assert(abs_max > 0);

    return pbio_math_bind(value, -abs_max, abs_max);
}

int32_t pbio_math_sqrt(int32_t n) {
    if (n <= 0) {
        return 0;
    }
    int32_t x0 = n;
    int32_t x1 = x0;

    while (true) {
        x1 = (x0 + n / x0) / 2;
        if (x1 == x0 || x1 == x0 + 1) {
            return x0;
        }
        x0 = x1;
    }
}

/**
 * Points on a curve for linear interpolation.
 */
typedef struct {
    int16_t x;
    int16_t y;
} point_t;

/**
 * Sample points along the curve y = atan(x / 1024) * 8.
 *
 * This is used to get the atan2(b, a) curve in the first quadrant. The
 * intermediate scaling is used to avoid excessive rounding errors.
 *
 * x = Ratios b / a, upscaled by 1024.
 * y = Matching atan(b / b) output, upscaled by 8 * 180 / pi (eighth of a degree).
 */
static const point_t atan_points[] = {
    { .x = 0, .y = 0 },
    { .x = 409, .y = 178 },
    { .x = 972, .y = 348 },
    { .x = 1638, .y = 472 },
    { .x = 2560, .y = 545 },
    { .x = 3891, .y = 605 },
    { .x = 5120, .y = 632 },
    { .x = 7168, .y = 660 },
    { .x = 15360, .y = 692 },
    { .x = 25600, .y = 705 },
};

/**
 * Interpolates a constant set of (X, Y) sample points on a curve y = f(x)
 * to estimate y for given input value x.
 *
 * If x < x[first] then it returns y[first].
 * If x >= x[last] then it returns y[last].
 *
 * @param [in]   points  Data points between which to interpolate.
 * @param [in]   len     Number of data points.
 * @param [in]   x       Value for which to estimate y = f(x)
 * @return               Estimated value for y = f(x)
 */
static int32_t pbio_math_interpolate(const point_t *points, size_t len, int32_t x) {

    // If x is below the minimum x, return the minimum y.
    if (x < points[0].x) {
        return points[0].y;
    }

    // Find nearest match and interpolate.
    for (size_t i = 0; i < len - 1; i++) {
        const point_t *p0 = &points[i];
        const point_t *p1 = &points[i + 1];

        if (x < p1->x) {
            return p0->y + (x - p0->x) * (p1->y - p0->y) / (p1->x - p0->x);
        }
    }

    // If x is below the maximum x, return the maximum y.
    return points[len - 1].y;
}

/**
 * Gets atan2 in degrees from integer inputs.
 *
 * @param [in]  y  Opposite side of the triangle.
 * @param [in]  x  Adjacent side of the triangle.
 * @return         atan2(y, x) in degrees.
 */
int32_t pbio_math_atan2(int32_t y, int32_t x) {

    // On y zero, the triangle is flat. Use X to find sign.
    if (y == 0) {
        return x > 0 ? 0 : -180;
    }

    // On x zero, the triangle height tends to infinity. Use y for sign.
    if (x == 0) {
        return 90 * pbio_math_sign(y);
    }

    // Get absolute ratio of y / x, upscaled to preserve resolution.
    int32_t ratio = 1024 * y / x;
    if (ratio < 0) {
        ratio = -ratio;
    }

    // Interpolate and scale to get corresponding atan value.
    int32_t atan = pbio_math_interpolate(atan_points, PBIO_ARRAY_SIZE(atan_points), ratio) / 8;

    // We took the absolute ratio, but must now account for sign.
    // So, negate if x and y had opposite sign.
    if ((x > 0) != (y > 0)) {
        atan = -atan;
    }

    // For small angles, we're done.
    if (x > 0) {
        return atan;
    }

    // Get atan2 result for larger angles.
    return atan > 0 ? atan - 180 : atan + 180;
}

/**
 * Multiplies two numbers and scales down the result.
 *
 * The result is @p a * @p b / @p c .
 *
 * The product of @p a and @p b must not exceed 2**47, and the result after
 * division must not exceed 2**31. @p c must not exceed 2**16.
 *
 * Adapted from https://stackoverflow.com/a/57727180, CC BY-SA 4.0
 *
 * @param [in]  a    Positive or negative number.
 * @param [in]  b    Positive or negative number.
 * @param [in]  c    Small positive or negative number.
 * @return           The result of a * b / c.
 */
int32_t pbio_math_mult_then_div(int32_t a, int32_t b, int32_t c) {

    // Get long product.
    uint64_t x = (uint64_t)(a < 0 ? -a : a) * (uint64_t)(b < 0 ? -b : b);

    assert(x < ((int64_t)1) << 48);
    assert(c != 0);

    uint32_t div = (c < 0 ? -c : c);

    // Set d1 to the high two base-65536 digits (bits 17 to 31) and d0 to
    // the low digit (bits 0 to 15).
    uint32_t d1 = x >> 16;
    uint32_t d0 = x & 0xffffu;

    //  Get the quotient and remainder of dividing d1 by div.
    uint32_t y1 = d1 / div;
    uint32_t r1 = d1 % div;

    // Combine previous remainder with the low digit of dividend and divide.
    uint32_t y0 = (r1 << 16 | d0) / div;

    // Return a quotient formed from the two quotient digits, signed by inputs.
    int32_t sign = (a < 0 ? -1 : 1) * (b < 0 ? -1 : 1) * (c < 0 ? -1 : 1);
    int32_t result = ((int32_t)(y1 << 16 | y0)) * sign;

    // Assert result is correct and return.
    assert(result == (int64_t)a * (int64_t)b / (int64_t)c);
    return result;
}
