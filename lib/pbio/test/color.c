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

    pbio_color_to_hsv(PBIO_COLOR_BLUE, &hsv);
    tt_want_int_op(hsv.h, ==, 240);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);

    pbio_color_to_hsv(PBIO_COLOR_PURPLE, &hsv);
    tt_want_int_op(hsv.h, ==, 270);
    tt_want_int_op(hsv.s, ==, 100);
    tt_want_int_op(hsv.v, ==, 100);
}
