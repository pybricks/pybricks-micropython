// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

#include <pbio/math.h>
#include <pbio/util.h>

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
 * Sample points along the cuve y = atan(x / 1024) * 8.
 *
 * This is used to get the atan2(b, a) curve in the first quadrant. The
 * intermediate scaling is used to avoid excessive roundoff errors.
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

    // On x zero, the triangle height tends to inifinity. Use y for sign.
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
