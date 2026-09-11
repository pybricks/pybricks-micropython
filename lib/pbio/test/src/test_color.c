// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <pbio/color.h>
#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

static void test_rgb_to_hsv(void *env) {
    pbio_color_rgb_t rgb;
    pbio_color_t hsv;

    // black
    rgb.r = 0;
    rgb.g = 0;
    rgb.b = 0;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(0, 0, 0));

    // white
    rgb.r = 255;
    rgb.g = 255;
    rgb.b = 255;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(0, 0, 100));

    // gray
    rgb.r = 127;
    rgb.g = 127;
    rgb.b = 127;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(0, 0, 50));

    // red
    rgb.r = 255;
    rgb.g = 0;
    rgb.b = 0;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(0, 100, 100));

    // green
    rgb.r = 0;
    rgb.g = 255;
    rgb.b = 0;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(120, 100, 100));

    // blue
    rgb.r = 0;
    rgb.g = 0;
    rgb.b = 255;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(240, 100, 100));

    // yellow
    rgb.r = 255;
    rgb.g = 255;
    rgb.b = 0;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(60, 100, 100));

    // cyan
    rgb.r = 0;
    rgb.g = 255;
    rgb.b = 255;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(180, 100, 100));

    // magenta
    rgb.r = 255;
    rgb.g = 0;
    rgb.b = 255;

    hsv = pbio_color_from_rgb(&rgb);
    tt_want_int_op(hsv, ==, PBIO_COLOR_ENCODE(300, 100, 100));
}

static void test_hsv_to_rgb(void *env) {
    pbio_color_rgb_t rgb;

    // no-color (negative brightness)
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(0, 0, -50), &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    // black
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(0, 0, 0), &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    // white
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(0, 0, 100), &rgb);
    tt_want_int_op(rgb.r, >, 250);
    tt_want_int_op(rgb.g, >, 250);
    tt_want_int_op(rgb.b, >, 250);

    // red
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(0, 100, 100), &rgb);
    tt_want_int_op(rgb.r, >, 250);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    // green
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(120, 100, 100), &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, >, 250);
    tt_want_int_op(rgb.b, ==, 0);

    // blue
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(240, 100, 100), &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, >, 250);

    // yellow
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(60, 100, 100), &rgb);
    tt_want_int_op(rgb.r, >, 120);
    tt_want_int_op(rgb.r, <, 130);
    tt_want_int_op(rgb.g, >, 120);
    tt_want_int_op(rgb.g, <, 130);
    tt_want_int_op(rgb.b, ==, 0);

    // cyan
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(180, 100, 100), &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, >, 120);
    tt_want_int_op(rgb.g, <, 130);
    tt_want_int_op(rgb.b, >, 120);
    tt_want_int_op(rgb.b, <, 130);

    // magenta
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(300, 100, 100), &rgb);
    tt_want_int_op(rgb.r, >, 120);
    tt_want_int_op(rgb.r, <, 130);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, >, 120);
    tt_want_int_op(rgb.b, <, 130);

    // orange
    pbio_color_to_rgb(PBIO_COLOR_ENCODE(30, 100, 100), &rgb);
    tt_want_int_op(rgb.r, >, 185);
    tt_want_int_op(rgb.r, <, 195);
    tt_want_int_op(rgb.g, >, 55);
    tt_want_int_op(rgb.g, <, 65);
    tt_want_int_op(rgb.b, ==, 0);
}

static void test_color_encoding(void *env) {

    tt_want_int_op(PBIO_COLOR_NONE, ==, PBIO_COLOR_ENCODE(180, 0, 0));

    tt_want_int_op(PBIO_COLOR_BLACK, ==, PBIO_COLOR_ENCODE(0, 0, 0));

    tt_want_int_op(PBIO_COLOR_GRAY, ==, PBIO_COLOR_ENCODE(0, 0, 50));

    tt_want_int_op(PBIO_COLOR_WHITE, ==, PBIO_COLOR_ENCODE(0, 0, 100));

    tt_want_int_op(PBIO_COLOR_RED, ==, PBIO_COLOR_ENCODE(0, 100, 100));

    tt_want_int_op(PBIO_COLOR_BROWN, ==, PBIO_COLOR_ENCODE(30, 100, 50));

    tt_want_int_op(PBIO_COLOR_ORANGE, ==, PBIO_COLOR_ENCODE(30, 100, 100));

    tt_want_int_op(PBIO_COLOR_YELLOW, ==, PBIO_COLOR_ENCODE(60, 100, 100));

    tt_want_int_op(PBIO_COLOR_GREEN, ==, PBIO_COLOR_ENCODE(120, 100, 100));

    tt_want_int_op(PBIO_COLOR_CYAN, ==, PBIO_COLOR_ENCODE(180, 100, 100));

    tt_want_int_op(PBIO_COLOR_BLUE, ==, PBIO_COLOR_ENCODE(240, 100, 100));

    tt_want_int_op(PBIO_COLOR_VIOLET, ==, PBIO_COLOR_ENCODE(270, 100, 100));

    tt_want_int_op(PBIO_COLOR_MAGENTA, ==, PBIO_COLOR_ENCODE(300, 100, 100));
}

static void test_color_to_rgb(void *env) {
    pbio_color_rgb_t rgb;

    pbio_color_to_rgb(PBIO_COLOR_NONE, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    pbio_color_to_rgb(PBIO_COLOR_BLACK, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    pbio_color_to_rgb(PBIO_COLOR_WHITE, &rgb);
    tt_want_int_op(rgb.r, >, 250);
    tt_want_int_op(rgb.g, >, 250);
    tt_want_int_op(rgb.b, >, 250);

    pbio_color_to_rgb(PBIO_COLOR_RED, &rgb);
    tt_want_int_op(rgb.r, >, 250);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    pbio_color_to_rgb(PBIO_COLOR_GREEN, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, >, 250);
    tt_want_int_op(rgb.b, ==, 0);

    pbio_color_to_rgb(PBIO_COLOR_BLUE, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, >, 250);

    pbio_color_to_rgb(PBIO_COLOR_YELLOW, &rgb);
    tt_want_int_op(rgb.r, >, 120);
    tt_want_int_op(rgb.r, <, 130);
    tt_want_int_op(rgb.g, >, 120);
    tt_want_int_op(rgb.g, <, 130);
    tt_want_int_op(rgb.b, ==, 0);

    pbio_color_to_rgb(PBIO_COLOR_CYAN, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, >, 120);
    tt_want_int_op(rgb.g, <, 130);
    tt_want_int_op(rgb.b, >, 120);
    tt_want_int_op(rgb.b, <, 130);

    pbio_color_to_rgb(PBIO_COLOR_MAGENTA, &rgb);
    tt_want_int_op(rgb.r, >, 120);
    tt_want_int_op(rgb.r, <, 130);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, >, 120);
    tt_want_int_op(rgb.b, <, 130);

    pbio_color_to_rgb(PBIO_COLOR_ORANGE, &rgb);
    tt_want_int_op(rgb.r, >, 185);
    tt_want_int_op(rgb.r, <, 195);
    tt_want_int_op(rgb.g, >, 55);
    tt_want_int_op(rgb.g, <, 65);
    tt_want_int_op(rgb.b, ==, 0);
}

static void test_color_hsv_cost(void *env) {
    pbio_color_t color_a;
    pbio_color_t color_b;
    int32_t dist;

    // color compared to itself should give 0
    color_a = PBIO_COLOR_ENCODE(0, 100, 100);
    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_a), ==, 0);

    // blacks with different saturations/hues should be the same
    color_a = PBIO_COLOR_ENCODE(230, 23, 0);

    color_b = PBIO_COLOR_ENCODE(23, 99, 0);
    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), ==, 0);

    // colors with different hues should be different when value>0 and saturation>0
    color_a = PBIO_COLOR_ENCODE(230, 99, 100);

    color_b = PBIO_COLOR_ENCODE(23, 99, 100);
    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), >, 0);

    // grays with different hues should be the same
    color_a = PBIO_COLOR_ENCODE(230, 0, 50);

    color_b = PBIO_COLOR_ENCODE(23, 0, 50);
    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), ==, 0);

    // distance should be greater when saturation is greater
    color_a = PBIO_COLOR_ENCODE(30, 20, 70);

    color_b = PBIO_COLOR_ENCODE(60, 20, 70);

    dist = pbio_color_get_distance_bicone_squared(color_a, color_b);

    color_a = PBIO_COLOR_ENCODE(30, 40, 70);

    color_b = PBIO_COLOR_ENCODE(60, 40, 70);

    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), >, dist);

    // resolve colors that are close
    color_a = PBIO_COLOR_ENCODE(30, 20, 70);

    color_b = PBIO_COLOR_ENCODE(35, 20, 70);

    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), >, 0);

    color_a = PBIO_COLOR_ENCODE(30, 20, 70);

    color_b = PBIO_COLOR_ENCODE(30, 25, 70);

    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), >, 0);

    color_a = PBIO_COLOR_ENCODE(30, 20, 70);

    color_b = PBIO_COLOR_ENCODE(30, 20, 75);

    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), >, 0);

    // hues 360 and 0 should be the same
    color_a = PBIO_COLOR_ENCODE(360, 100, 100);

    color_b = PBIO_COLOR_ENCODE(0, 100, 100);
    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), ==, 0);

    // distance between hues 359 and 1 should be smaller than hues 1 and 5
    color_a = PBIO_COLOR_ENCODE(359, 100, 100);

    color_b = PBIO_COLOR_ENCODE(1, 100, 100);
    dist = pbio_color_get_distance_bicone_squared(color_a, color_b);

    color_a = PBIO_COLOR_ENCODE(1, 100, 100);

    color_b = PBIO_COLOR_ENCODE(5, 100, 100);

    tt_want_int_op(pbio_color_get_distance_bicone_squared(color_a, color_b), >, dist);

    // check distance is monotonous along several color paths. This should catch potential int overflows
    int prev_dist = 0;
    bool monotone = true;

    // along saturation
    color_b = PBIO_COLOR_ENCODE(180, 0, 100);

    for (int s = 5; s <= 100; s += 5) {
        dist = pbio_color_get_distance_bicone_squared(PBIO_COLOR_ENCODE(180, s, 100), color_b);

        if (dist <= prev_dist) {
            monotone = false;
            break;
        }
        prev_dist = dist;
    }
    tt_want(monotone);

    // along value

    prev_dist = 0;
    monotone = true;

    color_b = PBIO_COLOR_ENCODE(180, 100, 0);

    for (int v = 5; v <= 100; v += 5) {
        dist = pbio_color_get_distance_bicone_squared(PBIO_COLOR_ENCODE(180, 100, v), color_b);

        if (dist <= prev_dist) {
            monotone = false;
            break;
        }
        prev_dist = dist;
    }
    tt_want(monotone);

    // along value, saturation 0

    prev_dist = 0;
    monotone = true;

    color_b = PBIO_COLOR_ENCODE(180, 0, 0);

    for (int v = 5; v <= 100; v += 5) {
        dist = pbio_color_get_distance_bicone_squared(PBIO_COLOR_ENCODE(180, 0, v), color_b);

        if (dist <= prev_dist) {
            monotone = false;
            break;
        }
        prev_dist = dist;
    }
    tt_want(monotone);

    // along chroma

    prev_dist = 0;
    monotone = true;

    color_b = PBIO_COLOR_ENCODE(180, 100, 100);

    for (int i = -19; i < 21; i++) {
        int s = i < 0 ? -i * 5 : i * 5;
        // constant lightness
        color_a = PBIO_COLOR_ENCODE(i < 0 ? 180 : 0, s, 10000 / (200 - s));

        dist = pbio_color_get_distance_bicone_squared(color_a, color_b);

        if (dist <= prev_dist) {
            monotone = false;
        }
        prev_dist = dist;
    }
    tt_want(monotone);

    // check max distances

    color_a = PBIO_COLOR_ENCODE(0, 100, 100);

    color_b = PBIO_COLOR_ENCODE(180, 100, 100);

    dist = pbio_color_get_distance_bicone_squared(color_a, color_b);
    tt_want_int_op(dist, >, 390000000);
    tt_want_int_op(dist, <, 410000000);

    color_a = PBIO_COLOR_ENCODE(0, 0, 0);

    color_b = PBIO_COLOR_ENCODE(0, 0, 100);

    dist = pbio_color_get_distance_bicone_squared(color_a, color_b);
    tt_want_int_op(dist, >, 390000000);
    tt_want_int_op(dist, <, 410000000);
}

struct testcase_t pbio_color_tests[] = {
    PBIO_TEST(test_rgb_to_hsv),
    PBIO_TEST(test_hsv_to_rgb),
    PBIO_TEST(test_color_encoding),
    PBIO_TEST(test_color_to_rgb),
    PBIO_TEST(test_color_hsv_cost),
    END_OF_TESTCASES
};
