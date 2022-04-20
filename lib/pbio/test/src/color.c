// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2021 The Pybricks Authors

#include <stdio.h>

#include <pbio/color.h>
#include <test-pbio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

static void test_rgb_to_hsv(void *env) {
    pbio_color_rgb_t rgb;
    pbio_color_hsv_t hsv;

    // black
    rgb.r = 0;
    rgb.g = 0;
    rgb.b = 0;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 0);
    tt_want_int_op(hsv.v, ==, 0);

    // white
    rgb.r = 255;
    rgb.g = 255;
    rgb.b = 255;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 0);
    tt_want_int_op(hsv.v, ==, 100);

    // gray
    rgb.r = 127;
    rgb.g = 127;
    rgb.b = 127;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 0);
    tt_want_int_op(hsv.v, ==, 50);

    // red
    rgb.r = 255;
    rgb.g = 0;
    rgb.b = 0;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    // green
    rgb.r = 0;
    rgb.g = 255;
    rgb.b = 0;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 120);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    // blue
    rgb.r = 0;
    rgb.g = 0;
    rgb.b = 255;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 240);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    // yellow
    rgb.r = 255;
    rgb.g = 255;
    rgb.b = 0;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 60);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    // cyan
    rgb.r = 0;
    rgb.g = 255;
    rgb.b = 255;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 180);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    // magenta
    rgb.r = 255;
    rgb.g = 0;
    rgb.b = 255;

    pbio_color_rgb_to_hsv(&rgb, &hsv);
    tt_want_int_op(hsv.h, ==, 300);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);
}

static void test_hsv_to_rgb(void *env) {
    pbio_color_hsv_t hsv;
    pbio_color_rgb_t rgb;

    // black
    hsv.h = 0;
    hsv.s = 0;
    hsv.v = 0;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    // white
    hsv.h = 0;
    hsv.s = 0;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, >, 250);
    tt_want_int_op(rgb.g, >, 250);
    tt_want_int_op(rgb.b, >, 250);

    // red
    hsv.h = 0;
    hsv.s = 100;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, >, 250);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, ==, 0);

    // green
    hsv.h = 120;
    hsv.s = 100;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, >, 250);
    tt_want_int_op(rgb.b, ==, 0);

    // blue
    hsv.h = 240;
    hsv.s = 100;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, >, 250);

    // yellow
    hsv.h = 60;
    hsv.s = 100;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, >, 120);
    tt_want_int_op(rgb.r, <, 130);
    tt_want_int_op(rgb.g, >, 120);
    tt_want_int_op(rgb.g, <, 130);
    tt_want_int_op(rgb.b, ==, 0);

    // cyan
    hsv.h = 180;
    hsv.s = 100;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, ==, 0);
    tt_want_int_op(rgb.g, >, 120);
    tt_want_int_op(rgb.g, <, 130);
    tt_want_int_op(rgb.b, >, 120);
    tt_want_int_op(rgb.b, <, 130);

    // magenta
    hsv.h = 300;
    hsv.s = 100;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, >, 120);
    tt_want_int_op(rgb.r, <, 130);
    tt_want_int_op(rgb.g, ==, 0);
    tt_want_int_op(rgb.b, >, 120);
    tt_want_int_op(rgb.b, <, 130);

    // orange
    hsv.h = 30;
    hsv.s = 100;
    hsv.v = 100;
    pbio_color_hsv_to_rgb(&hsv, &rgb);
    tt_want_int_op(rgb.r, >, 185);
    tt_want_int_op(rgb.r, <, 195);
    tt_want_int_op(rgb.g, >, 55);
    tt_want_int_op(rgb.g, <, 65);
    tt_want_int_op(rgb.b, ==, 0);
}

static void test_color_to_hsv(void *env) {
    pbio_color_hsv_t hsv;

    pbio_color_to_hsv(PBIO_COLOR_NONE, &hsv);
    tt_want_int_op(hsv.h, ==, 180);
    tt_want_int_op(hsv.s, ==, 0);
    tt_want_int_op(hsv.v, ==, 0);

    pbio_color_to_hsv(PBIO_COLOR_BLACK, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 0);
    tt_want_int_op(hsv.v, ==, 0);

    pbio_color_to_hsv(PBIO_COLOR_GRAY, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 0);
    tt_want_int_op(hsv.v, ==, 50);

    pbio_color_to_hsv(PBIO_COLOR_WHITE, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 0);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_RED, &hsv);
    tt_want_int_op(hsv.h, ==, 0);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_BROWN, &hsv);
    tt_want_int_op(hsv.h, ==, 30);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 50);

    pbio_color_to_hsv(PBIO_COLOR_ORANGE, &hsv);
    tt_want_int_op(hsv.h, ==, 30);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_YELLOW, &hsv);
    tt_want_int_op(hsv.h, ==, 60);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_GREEN, &hsv);
    tt_want_int_op(hsv.h, ==, 120);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_CYAN, &hsv);
    tt_want_int_op(hsv.h, ==, 180);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_BLUE, &hsv);
    tt_want_int_op(hsv.h, ==, 240);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_VIOLET, &hsv);
    tt_want_int_op(hsv.h, ==, 270);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_MAGENTA, &hsv);
    tt_want_int_op(hsv.h, ==, 300);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);
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

static void test_color_hsv_compression(void *env) {
    // if this isn't true, we aren't compressing anything
    tt_want_int_op(sizeof(pbio_color_compressed_hsv_t), <, sizeof(pbio_color_hsv_t));

    const pbio_color_hsv_t hsv = { .h = PBIO_COLOR_HUE_CYAN, .s = 100, .v = 75 };

    pbio_color_compressed_hsv_t compressed;
    pbio_color_hsv_compress(&hsv, &compressed);
    tt_want_int_op(hsv.h, ==, compressed.h);
    tt_want_int_op(hsv.s, ==, compressed.s);
    tt_want_int_op(hsv.v, ==, compressed.v);

    pbio_color_hsv_t expanded;
    pbio_color_hsv_expand(&compressed, &expanded);
    tt_want_int_op(hsv.h, ==, expanded.h);
    tt_want_int_op(hsv.s, ==, expanded.s);
    tt_want_int_op(hsv.v, ==, expanded.v);
}

static void test_color_hsv_cost(void *env) {
    pbio_color_hsv_t color_a;
    pbio_color_hsv_t color_b;
    int32_t dist;
    int32_t chroma_weight = 50;

    // color compared to itself should give 0
    color_a.h = 0;
    color_a.s = 100;
    color_a.v = 100;
    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_a, chroma_weight), ==, 0);

    // blacks with different saturations/hues should be the same
    color_a.h = 230;
    color_a.s = 23;
    color_a.v = 0;

    color_b.h = 23;
    color_b.s = 99;
    color_b.v = 0;
    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), ==, 0);

    // colors with different hues should be different when value>0 and saturation>0
    color_a.h = 230;
    color_a.s = 99;
    color_a.v = 100;

    color_b.h = 23;
    color_b.s = 99;
    color_b.v = 100;
    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), >, 0);

    // grays with different hues should be the same
    color_a.h = 230;
    color_a.s = 0;
    color_a.v = 50;

    color_b.h = 23;
    color_b.s = 0;
    color_b.v = 50;
    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), ==, 0);

    // distance should be greater when saturation is greater
    color_a.h = 30;
    color_a.s = 20;
    color_a.v = 70;

    color_b.h = 60;
    color_b.s = 20;
    color_b.v = 70;

    dist = pbio_get_cone_cost(&color_a, &color_b, chroma_weight);

    color_a.h = 30;
    color_a.s = 40;
    color_a.v = 70;

    color_b.h = 60;
    color_b.s = 40;
    color_b.v = 70;

    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), >, dist);

    // resolve colors that are very close
    color_a.h = 30;
    color_a.s = 20;
    color_a.v = 70;

    color_b.h = 35;
    color_b.s = 20;
    color_b.v = 70;

    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), >, 0);

    color_a.h = 30;
    color_a.s = 20;
    color_a.v = 70;

    color_b.h = 30;
    color_b.s = 25;
    color_b.v = 70;

    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), >, 0);

    color_a.h = 30;
    color_a.s = 20;
    color_a.v = 70;

    color_b.h = 30;
    color_b.s = 20;
    color_b.v = 71;

    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), >, 0);

    // distance of opposite colors should be the same as double saturation to gray
    color_a.h = 20;
    color_a.s = 20;
    color_a.v = 70;

    color_b.h = 200;
    color_b.s = 20;
    color_b.v = 70;
    dist = pbio_get_cone_cost(&color_a, &color_b, chroma_weight);

    color_a.h = 20;
    color_a.s = 40;
    color_a.v = 70;

    color_b.h = 0;
    color_b.s = 0;
    color_b.v = 70;

    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), ==, dist);

    // hues 360 and 0 should be the same
    color_a.h = 360;
    color_a.s = 100;
    color_a.v = 100;

    color_b.h = 0;
    color_b.s = 100;
    color_b.v = 100;
    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), ==, 0);

    // distance between hues 359 and 1 should be smaller than hues 1 and 5
    color_a.h = 359;
    color_a.s = 100;
    color_a.v = 100;

    color_b.h = 1;
    color_b.s = 100;
    color_b.v = 100;
    dist = pbio_get_cone_cost(&color_a, &color_b, chroma_weight);

    color_a.h = 1;
    color_a.s = 100;
    color_a.v = 100;

    color_b.h = 5;
    color_b.s = 100;
    color_b.v = 100;

    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, chroma_weight), >, dist);

    // when chroma_weight is 0, all colors with same values should be the same
    color_a.h = 0;
    color_a.s = 100;
    color_a.v = 100;

    color_b.h = 180;
    color_b.s = 100;
    color_b.v = 100;
    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, 0), ==, 0);

    // when chroma_weight is 100, all colors with same hue and chroma (s*v) should be the same
    color_a.h = 0;
    color_a.s = 100;
    color_a.v = 50;

    color_b.h = 0;
    color_b.s = 50;
    color_b.v = 100;
    tt_want_int_op(pbio_get_cone_cost(&color_a, &color_b, 100), ==, 0);


}

struct testcase_t pbio_color_tests[] = {
    PBIO_TEST(test_rgb_to_hsv),
    PBIO_TEST(test_hsv_to_rgb),
    PBIO_TEST(test_color_to_hsv),
    PBIO_TEST(test_color_to_rgb),
    PBIO_TEST(test_color_hsv_compression),
    PBIO_TEST(test_color_hsv_cost),
    END_OF_TESTCASES
};
