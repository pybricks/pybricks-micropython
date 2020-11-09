// SPDX-License-Identifier: MIT
// Copyright 2020 The Pybricks Authors

#include <stdio.h>

#include <pbio/color.h>

#include <tinytest.h>
#include <tinytest_macros.h>

void test_rgb_to_hsv(void *env) {
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

void test_hsv_to_rgb(void *env) {
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

void test_color_to_hsv(void *env) {
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

void test_color_to_rgb(void *env) {
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

void test_color_hsv_compression(void *env) {
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
