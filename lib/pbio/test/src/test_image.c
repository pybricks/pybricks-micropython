// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tinytest.h>
#include <tinytest_macros.h>
#include <test-pbio.h>

#include <pbio/image.h>

// Tests use an outer image and an inner image which is a sub-image of the
// outer image.
//
// Drawing is done in the inner image and clipping should prevent the drawing
// function to touch outer image pixels. This is checked by clipping tests.
//
// The same drawing is done in a image of the same size of the outer image.
// The inner image must be identical to the corresponding part in the large
// image. This is done to test that clipping does not change the way visible
// pixels are drawn.
#define OUTER_IMAGE_WIDTH 400
#define OUTER_IMAGE_HEIGHT 256
#define INNER_IMAGE_X 100
#define INNER_IMAGE_Y 64
#define INNER_IMAGE_WIDTH 200
#define INNER_IMAGE_HEIGHT 128

// Other tests use a small image so that the expected result can be embedded
// in the test code using a readable test string. Text strings can be used for
// the comparison as a single byte is used per pixel.
#define SMALL_IMAGE_WIDTH 20
#define SMALL_IMAGE_HEIGHT 7

static uint8_t test_image_pixels[2][OUTER_IMAGE_HEIGHT][OUTER_IMAGE_WIDTH];

static uint8_t test_image_small_pixels[SMALL_IMAGE_HEIGHT][SMALL_IMAGE_WIDTH];

static void test_image_prepare_images(pbio_image_t *outer, pbio_image_t
    *inner, pbio_image_t *full) {
    memset(test_image_pixels, 0xff, sizeof(test_image_pixels));
    pbio_image_init(outer, &test_image_pixels[0][0][0], OUTER_IMAGE_WIDTH,
        OUTER_IMAGE_HEIGHT, OUTER_IMAGE_WIDTH);
    pbio_image_init_sub(inner, outer, INNER_IMAGE_X, INNER_IMAGE_Y,
        INNER_IMAGE_WIDTH, INNER_IMAGE_HEIGHT);
    pbio_image_init(full, &test_image_pixels[1][0][0], OUTER_IMAGE_WIDTH,
        OUTER_IMAGE_HEIGHT, OUTER_IMAGE_WIDTH);
    pbio_image_fill(inner, 0);
    pbio_image_fill(full, 0);
}

static bool test_image_clipping_correct(void) {
    int x, y;
    for (y = 0; y < INNER_IMAGE_Y; y++) {
        for (x = 0; x < OUTER_IMAGE_WIDTH; x++) {
            if (test_image_pixels[0][y][x] != 0xff) {
                return false;
            }
        }
    }
    for (; y < INNER_IMAGE_Y + INNER_IMAGE_HEIGHT; y++) {
        for (x = 0; x < INNER_IMAGE_X; x++) {
            if (test_image_pixels[0][y][x] != 0xff) {
                return false;
            }
        }
        for (; x < INNER_IMAGE_X + INNER_IMAGE_WIDTH; x++) {
            if (test_image_pixels[0][y][x] != test_image_pixels[1][y][x]) {
                return false;
            }
        }
        for (; x < OUTER_IMAGE_WIDTH; x++) {
            if (test_image_pixels[0][y][x] != 0xff) {
                return false;
            }
        }
    }
    for (; y < OUTER_IMAGE_HEIGHT; y++) {
        for (x = 0; x < OUTER_IMAGE_WIDTH; x++) {
            if (test_image_pixels[0][y][x] != 0xff) {
                return false;
            }
        }
    }
    return true;
}

static void test_image_write_pgm(const char *filename, int width, int height,
    uint8_t *pixels) {
    FILE *f;

    f = fopen(filename, "wb");
    if (f) {
        fprintf(f, "P5\n%d %d %d\n", width, height, 255);
        fwrite(pixels, width, height, f);
        fclose(f);
    }
}

static void test_image_dump(void) {
    static bool dumped = false;

    // Only dump first failed test, only if verbose.
    if (tinytest_get_verbosity_() <= 1) {
        printf("\n  (use --verbose to dump)");
        return;
    }
    if (dumped) {
        return;
    }
    dumped = true;

    test_image_write_pgm("outer.pgm", OUTER_IMAGE_WIDTH, OUTER_IMAGE_HEIGHT,
        &test_image_pixels[0][0][0]);
    test_image_write_pgm("full.pgm", OUTER_IMAGE_WIDTH, OUTER_IMAGE_HEIGHT,
        &test_image_pixels[1][0][0]);
    printf("\n  (dumped to outer.pgm and full.pgm)");
}

#define tt_want_clipping_correct() \
    tt_want_(test_image_clipping_correct(), "want(clipping correct)", \
    test_image_dump())

static void test_image_prepare_small_image(pbio_image_t *small) {
    memset(test_image_small_pixels, '.', sizeof(test_image_small_pixels));
    pbio_image_init(small, &test_image_small_pixels[0][0], SMALL_IMAGE_WIDTH,
        SMALL_IMAGE_HEIGHT, SMALL_IMAGE_WIDTH);
}

static bool test_image_small_equal(const char *expected) {
    return memcmp(test_image_small_pixels, expected,
        sizeof(test_image_small_pixels)) == 0;
}

static void test_image_small_diff(const char *expected) {
    printf("\n    %-*s  %-*s", SMALL_IMAGE_WIDTH, "result",
        SMALL_IMAGE_WIDTH, "expected");
    for (int y = 0; y < SMALL_IMAGE_HEIGHT; y++) {
        printf("\n    %.*s  %.*s", SMALL_IMAGE_WIDTH,
            test_image_small_pixels[y], SMALL_IMAGE_WIDTH,
            expected + SMALL_IMAGE_WIDTH * y);
    }
}

#define tt_want_small_image(expected) \
    tt_want_(test_image_small_equal((expected)), "want(image match)", \
    test_image_small_diff((expected)))

static void test_image_fill(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_fill(&small, '*');
    tt_want_small_image(
        "********************"
        "********************"
        "********************"
        "********************"
        "********************"
        "********************"
        "********************");

    test_image_prepare_images(&outer, &inner, &full);
    pbio_image_fill(&inner, 1);
    pbio_image_fill(&full, 1);
    tt_want_clipping_correct();
}

static void test_image_draw_image(void *env) {
    pbio_image_t small, outer, inner, full;
    pbio_image_t stamp, large_stamp;

    pbio_image_init(&stamp, (uint8_t *)
        " +-+ "
        " | | "
        "====="
        "STAMP", 5, 4, 5);
    pbio_image_init(&large_stamp, (uint8_t *)
        "STAMP stamp ST.......mp STAMP stamp", 27, 8, 1);

    test_image_prepare_small_image(&small);
    pbio_image_draw_image(&small, &stamp, 1, 1);
    pbio_image_draw_image(&small, &stamp, 5, 3);
    pbio_image_draw_image(&small, &stamp, 12, 0);
    tt_want_small_image(
        "............ +-+ ..."
        ". +-+ ...... | | ..."
        ". | | ......=====..."
        ".==== +-+ ..STAMP..."
        ".STAM | | .........."
        ".....=====.........."
        ".....STAMP..........");

    test_image_prepare_small_image(&small);
    pbio_image_draw_image_transparent(&small, &stamp, 1, 1, ' ');
    pbio_image_draw_image_transparent(&small, &stamp, 5, 3, ' ');
    pbio_image_draw_image_transparent(&small, &stamp, 12, 0, ' ');
    pbio_image_draw_image_transparent(&small, &stamp, 12, 2, ' ');
    tt_want_small_image(
        ".............+-+...."
        "..+-+........|.|...."
        "..|.|.......=+-+=..."
        ".=====+-+...S|A|P..."
        ".STAMP|.|...=====..."
        ".....=====..STAMP..."
        ".....STAMP..........");

    test_image_prepare_small_image(&small);
    pbio_image_draw_image(&small, &large_stamp, -3, 0);
    tt_want_small_image(
        "MP stamp ST.......mp"
        "P stamp ST.......mp "
        " stamp ST.......mp S"
        "stamp ST.......mp ST"
        "tamp ST.......mp STA"
        "amp ST.......mp STAM"
        "mp ST.......mp STAMP");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % (OUTER_IMAGE_WIDTH - 4) + 2;
        int y = rand() % (OUTER_IMAGE_HEIGHT - 4) + 2;
        if (i % 2) {
            pbio_image_draw_image(&inner, &stamp,
                x - INNER_IMAGE_X, y - INNER_IMAGE_Y);
            pbio_image_draw_image(&full, &stamp, x, y);
        } else {
            pbio_image_draw_image_transparent(&inner, &stamp,
                x - INNER_IMAGE_X, y - INNER_IMAGE_Y, ' ');
            pbio_image_draw_image_transparent(&full, &stamp, x, y, ' ');
        }
    }
    tt_want_clipping_correct();
}

static void test_image_draw_pixel(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_draw_pixel(&small, 1, 1, '*');
    pbio_image_draw_pixel(&small, 18, 5, '#');
    tt_want_small_image(
        "...................."
        ".*.................."
        "...................."
        "...................."
        "...................."
        "..................#."
        "....................");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % (OUTER_IMAGE_WIDTH - 4) + 2;
        int y = rand() % (OUTER_IMAGE_HEIGHT - 4) + 2;
        uint8_t value = rand() & 0xff;
        pbio_image_draw_pixel(&inner, x - INNER_IMAGE_X, y - INNER_IMAGE_Y,
            value);
        pbio_image_draw_pixel(&full, x, y, value);
    }
    tt_want_clipping_correct();
}

static void test_image_draw_line(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_draw_line(&small, 1, 2, 18, 2, '*');
    pbio_image_draw_line(&small, 18, 4, 1, 4, '#');
    tt_want_small_image(
        "...................."
        "...................."
        ".******************."
        "...................."
        ".##################."
        "...................."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_line(&small, 1, 1, 1, 5, '*');
    pbio_image_draw_line(&small, 3, 5, 3, 1, '#');
    tt_want_small_image(
        "...................."
        ".*.#................"
        ".*.#................"
        ".*.#................"
        ".*.#................"
        ".*.#................"
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_line(&small, 1, 1, 18, 5, '*');
    tt_want_small_image(
        "...................."
        ".***................"
        "....****............"
        "........****........"
        "............****...."
        "................***."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_line(&small, 18, 5, 1, 1, '*');
    tt_want_small_image(
        "...................."
        ".***................"
        "....****............"
        "........****........"
        "............****...."
        "................***."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_line(&small, 1, 4, 18, 0, '*');
    pbio_image_draw_line(&small, 18, 2, 1, 6, '#');
    tt_want_small_image(
        "................***."
        "............****...."
        "........****....###."
        "....****....####...."
        ".***....####........"
        "....####............"
        ".###................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_line(&small, 2, 1, 4, 5, '*');
    pbio_image_draw_line(&small, 7, 5, 5, 1, '*');
    pbio_image_draw_line(&small, 11, 5, 13, 1, '#');
    pbio_image_draw_line(&small, 16, 1, 14, 5, '#');
    tt_want_small_image(
        "...................."
        "..*..*.......#..#..."
        "...*..*.....#..#...."
        "...*..*.....#..#...."
        "....*..*...#..#....."
        "....*..*...#..#....."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_line(&small, 1, 1, 5, 5, '*');
    pbio_image_draw_line(&small, 8, 5, 4, 1, '*');
    pbio_image_draw_line(&small, 10, 5, 14, 1, '#');
    pbio_image_draw_line(&small, 17, 1, 13, 5, '#');
    tt_want_small_image(
        "...................."
        ".*..*.........#..#.."
        "..*..*.......#..#..."
        "...*..*.....#..#...."
        "....*..*...#..#....."
        ".....*..*.#..#......"
        "....................");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x1 = rand() % (OUTER_IMAGE_WIDTH - 4) + 2;
        int y1 = rand() % (OUTER_IMAGE_HEIGHT - 4) + 2;
        int x2 = rand() % (OUTER_IMAGE_WIDTH - 4) + 2;
        int y2 = rand() % (OUTER_IMAGE_HEIGHT - 4) + 2;
        uint8_t value = rand() & 0xff;
        pbio_image_draw_line(&inner, x1 - INNER_IMAGE_X, y1 - INNER_IMAGE_Y,
            x2 - INNER_IMAGE_X, y2 - INNER_IMAGE_Y, value);
        pbio_image_draw_line(&full, x1, y1, x2, y2, value);
    }
    tt_want_clipping_correct();
}

static void test_image_draw_thick_line(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 1, 2, 18, 2, 2, '*');
    pbio_image_draw_thick_line(&small, 18, 4, 1, 4, 2, '#');
    tt_want_small_image(
        "...................."
        ".******************."
        ".******************."
        "...................."
        ".##################."
        ".##################."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 1, 1, 1, 5, 2, '*');
    pbio_image_draw_thick_line(&small, 6, 5, 6, 1, 2, '#');
    tt_want_small_image(
        "...................."
        ".**..##............."
        ".**..##............."
        ".**..##............."
        ".**..##............."
        ".**..##............."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 1, 1, 18, 5, 2, '*');
    tt_want_small_image(
        ".***................"
        ".*******............"
        "....********........"
        "........********...."
        "............*******."
        "................***."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 18, 5, 1, 1, 2, '*');
    tt_want_small_image(
        "...................."
        ".***................"
        ".*******............"
        "....********........"
        "........********...."
        "............*******."
        "................***.");

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 1, 4, 18, 0, 2, '*');
    pbio_image_draw_thick_line(&small, 18, 2, 1, 6, 2, '#');
    tt_want_small_image(
        "............*******."
        "........********...."
        "....********....###."
        ".*******....#######."
        ".***....########...."
        "....########........"
        ".#######............");

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 2, 1, 4, 5, 2, '*');
    pbio_image_draw_thick_line(&small, 8, 5, 6, 1, 2, '*');
    pbio_image_draw_thick_line(&small, 12, 5, 14, 1, 2, '#');
    pbio_image_draw_thick_line(&small, 17, 1, 15, 5, 2, '#');
    tt_want_small_image(
        "...................."
        "..**.**......##..##."
        "...**.**....##..##.."
        "...**.**....##..##.."
        "....**.**..##..##..."
        "....**.**..##..##..."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 1, 1, 5, 5, 2, '*');
    pbio_image_draw_thick_line(&small, 9, 5, 5, 1, 2, '*');
    pbio_image_draw_thick_line(&small, 11, 5, 15, 1, 2, '#');
    pbio_image_draw_thick_line(&small, 18, 1, 14, 5, 2, '#');
    tt_want_small_image(
        ".*.............#...."
        ".**..*........##..#."
        "..**.**......##..##."
        "...**.**....##..##.."
        "....**.**..##..##..."
        ".....*..**.#..##...."
        ".........*....#.....");

    test_image_prepare_small_image(&small);
    pbio_image_draw_thick_line(&small, 1, 3, 18, 3, 7, '-');
    pbio_image_draw_thick_line(&small, 1, 3, 18, 3, 5, '+');
    pbio_image_draw_thick_line(&small, 1, 3, 18, 3, 3, '*');
    pbio_image_draw_thick_line(&small, 1, 3, 18, 3, 1, '#');
    tt_want_small_image(
        ".------------------."
        ".++++++++++++++++++."
        ".******************."
        ".##################."
        ".******************."
        ".++++++++++++++++++."
        ".------------------.");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x1 = rand() % (OUTER_IMAGE_WIDTH - 4) + 2;
        int y1 = rand() % (OUTER_IMAGE_HEIGHT - 4) + 2;
        int x2 = rand() % (OUTER_IMAGE_WIDTH - 4) + 2;
        int y2 = rand() % (OUTER_IMAGE_HEIGHT - 4) + 2;
        int thickness = rand() % 5 + 1;
        uint8_t value = rand() & 0xff;
        pbio_image_draw_thick_line(&inner, x1 - INNER_IMAGE_X,
            y1 - INNER_IMAGE_Y, x2 - INNER_IMAGE_X, y2 - INNER_IMAGE_Y,
            thickness, value);
        pbio_image_draw_thick_line(&full, x1, y1, x2, y2, thickness, value);
    }
    tt_want_clipping_correct();
}

static void test_image_draw_rect(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_draw_rect(&small, 1, 1, 18, 5, '*');
    tt_want_small_image(
        "...................."
        ".******************."
        ".*................*."
        ".*................*."
        ".*................*."
        ".******************."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_rect(&small, 1, 1, 18, 1, '*');
    pbio_image_draw_rect(&small, 1, 3, 18, 2, '#');
    pbio_image_draw_rect(&small, 10, 1, 1, 6, '+');
    pbio_image_draw_rect(&small, 13, 1, 2, 6, '/');
    tt_want_small_image(
        "...................."
        ".*********+**//****."
        "..........+..//....."
        ".#########+##//####."
        ".#########+##//####."
        "..........+..//....."
        "..........+..//.....");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % OUTER_IMAGE_WIDTH;
        int y = rand() % OUTER_IMAGE_HEIGHT;
        int width = rand() % (OUTER_IMAGE_WIDTH / 5);
        int height = rand() % (OUTER_IMAGE_HEIGHT / 5);
        uint8_t value = rand() & 0xff;
        pbio_image_draw_rect(&inner, x - INNER_IMAGE_X, y - INNER_IMAGE_Y,
            width, height, value);
        pbio_image_draw_rect(&full, x, y, width, height, value);
    }
    tt_want_clipping_correct();
}

static void test_image_fill_rect(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_fill_rect(&small, 1, 1, 18, 5, '*');
    tt_want_small_image(
        "...................."
        ".******************."
        ".******************."
        ".******************."
        ".******************."
        ".******************."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_fill_rect(&small, 1, 1, 18, 1, '*');
    pbio_image_fill_rect(&small, 1, 3, 18, 2, '#');
    pbio_image_fill_rect(&small, 10, 1, 1, 6, '+');
    pbio_image_fill_rect(&small, 13, 1, 2, 6, '/');
    tt_want_small_image(
        "...................."
        ".*********+**//****."
        "..........+..//....."
        ".#########+##//####."
        ".#########+##//####."
        "..........+..//....."
        "..........+..//.....");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % OUTER_IMAGE_WIDTH;
        int y = rand() % OUTER_IMAGE_HEIGHT;
        int width = rand() % (OUTER_IMAGE_WIDTH / 5);
        int height = rand() % (OUTER_IMAGE_HEIGHT / 5);
        uint8_t value = rand() & 0xff;
        pbio_image_fill_rect(&inner, x - INNER_IMAGE_X, y - INNER_IMAGE_Y,
            width, height, value);
        pbio_image_fill_rect(&full, x, y, width, height, value);
    }
    tt_want_clipping_correct();
}

static void test_image_draw_rounded_rect(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_draw_rounded_rect(&small, 1, 1, 18, 5, 1, '*');
    tt_want_small_image(
        "...................."
        "..****************.."
        ".*................*."
        ".*................*."
        ".*................*."
        "..****************.."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_rounded_rect(&small, 1, 1, 18, 5, 2, '*');
    tt_want_small_image(
        "...................."
        "...**************..."
        "..*..............*.."
        ".*................*."
        "..*..............*.."
        "...**************..."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_rounded_rect(&small, 0, 0, 20, 7, 3, '*');
    tt_want_small_image(
        "...**************..."
        ".**..............**."
        ".*................*."
        "*..................*"
        ".*................*."
        ".**..............**."
        "...**************...");

    test_image_prepare_small_image(&small);
    pbio_image_draw_rounded_rect(&small, 0, 0, 40, 40, 6, '*');
    tt_want_small_image(
        "......**************"
        "...***.............."
        "..*................."
        ".*.................."
        ".*.................."
        ".*.................."
        "*...................");

    test_image_prepare_small_image(&small);
    pbio_image_draw_rounded_rect(&small, 1, 1, 18, 1, 1, '*');
    pbio_image_draw_rounded_rect(&small, 1, 3, 18, 2, 1, '#');
    pbio_image_draw_rounded_rect(&small, 10, 1, 1, 6, 1, '+');
    pbio_image_draw_rounded_rect(&small, 13, 1, 2, 6, 1, '/');
    tt_want_small_image(
        "...................."
        ".*********+**//****."
        "..........+..//....."
        ".#########+##//####."
        ".#########+##//####."
        "..........+..//....."
        "..........+..//.....");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % OUTER_IMAGE_WIDTH;
        int y = rand() % OUTER_IMAGE_HEIGHT;
        int width = rand() % (OUTER_IMAGE_WIDTH / 5);
        int height = rand() % (OUTER_IMAGE_HEIGHT / 5);
        int r = rand() % (OUTER_IMAGE_HEIGHT / 25);
        uint8_t value = rand() & 0xff;
        pbio_image_draw_rounded_rect(&inner, x - INNER_IMAGE_X,
            y - INNER_IMAGE_Y, width, height, r, value);
        pbio_image_draw_rounded_rect(&full, x, y, width, height, r, value);
    }
    tt_want_clipping_correct();
}

static void test_image_fill_rounded_rect(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_fill_rounded_rect(&small, 1, 1, 18, 5, 1, '*');
    tt_want_small_image(
        "...................."
        "..****************.."
        ".******************."
        ".******************."
        ".******************."
        "..****************.."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_fill_rounded_rect(&small, 1, 1, 18, 5, 10, '*');
    tt_want_small_image(
        "...................."
        "...**************..."
        "..****************.."
        ".******************."
        "..****************.."
        "...**************..."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_fill_rounded_rect(&small, 1, 1, 18, 5, 2, '*');
    tt_want_small_image(
        "...................."
        "...**************..."
        "..****************.."
        ".******************."
        "..****************.."
        "...**************..."
        "....................");

    test_image_prepare_small_image(&small);
    pbio_image_fill_rounded_rect(&small, 0, 0, 20, 7, 3, '*');
    tt_want_small_image(
        "...**************..."
        ".******************."
        ".******************."
        "********************"
        ".******************."
        ".******************."
        "...**************...");

    test_image_prepare_small_image(&small);
    pbio_image_fill_rounded_rect(&small, 0, 0, 40, 40, 6, '*');
    tt_want_small_image(
        "......**************"
        "...*****************"
        "..******************"
        ".*******************"
        ".*******************"
        ".*******************"
        "********************");

    test_image_prepare_small_image(&small);
    pbio_image_fill_rounded_rect(&small, 1, 1, 18, 1, 1, '*');
    pbio_image_fill_rounded_rect(&small, 1, 3, 18, 2, 1, '#');
    pbio_image_fill_rounded_rect(&small, 10, 1, 1, 6, 1, '+');
    pbio_image_fill_rounded_rect(&small, 13, 1, 2, 6, 1, '/');
    tt_want_small_image(
        "...................."
        ".*********+**//****."
        "..........+..//....."
        ".#########+##//####."
        ".#########+##//####."
        "..........+..//....."
        "..........+..//.....");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % OUTER_IMAGE_WIDTH;
        int y = rand() % OUTER_IMAGE_HEIGHT;
        int width = rand() % (OUTER_IMAGE_WIDTH / 5);
        int height = rand() % (OUTER_IMAGE_HEIGHT / 5);
        int r = rand() % (OUTER_IMAGE_HEIGHT / 25);
        uint8_t value = rand() & 0xff;
        pbio_image_fill_rounded_rect(&inner, x - INNER_IMAGE_X,
            y - INNER_IMAGE_Y, width, height, r, value);
        pbio_image_fill_rounded_rect(&full, x, y, width, height, r, value);
    }
    tt_want_clipping_correct();
}

static void test_image_draw_circle(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_draw_circle(&small, 3, 3, 3, '*');
    tt_want_small_image(
        "..***..............."
        ".*...*.............."
        "*.....*............."
        "*.....*............."
        "*.....*............."
        ".*...*.............."
        "..***...............");

    test_image_prepare_small_image(&small);
    pbio_image_draw_circle(&small, 6, 6, 6, '*');
    tt_want_small_image(
        "....*****..........."
        "...*.....*.........."
        "..*.......*........."
        ".*.........*........"
        "*...........*......."
        "*...........*......."
        "*...........*.......");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % OUTER_IMAGE_WIDTH;
        int y = rand() % OUTER_IMAGE_HEIGHT;
        int r = rand() % (OUTER_IMAGE_HEIGHT / 5);
        uint8_t value = rand() & 0xff;
        pbio_image_draw_circle(&inner, x - INNER_IMAGE_X,
            y - INNER_IMAGE_Y, r, value);
        pbio_image_draw_circle(&full, x, y, r, value);
    }
    tt_want_clipping_correct();
}

static void test_image_fill_circle(void *env) {
    pbio_image_t small, outer, inner, full;

    test_image_prepare_small_image(&small);
    pbio_image_fill_circle(&small, 3, 3, 3, '*');
    tt_want_small_image(
        "..***..............."
        ".*****.............."
        "*******............."
        "*******............."
        "*******............."
        ".*****.............."
        "..***...............");

    test_image_prepare_small_image(&small);
    pbio_image_fill_circle(&small, 6, 6, 6, '*');
    tt_want_small_image(
        "....*****..........."
        "...*******.........."
        "..*********........."
        ".***********........"
        "*************......."
        "*************......."
        "*************.......");

    test_image_prepare_images(&outer, &inner, &full);
    srand(1234);
    for (int i = 0; i < 10000; i++) {
        int x = rand() % OUTER_IMAGE_WIDTH;
        int y = rand() % OUTER_IMAGE_HEIGHT;
        int r = rand() % (OUTER_IMAGE_HEIGHT / 5);
        uint8_t value = rand() & 0xff;
        pbio_image_fill_circle(&inner, x - INNER_IMAGE_X,
            y - INNER_IMAGE_Y, r, value);
        pbio_image_fill_circle(&full, x, y, r, value);
    }
    tt_want_clipping_correct();
}

struct testcase_t pbio_image_tests[] = {
    PBIO_TEST(test_image_fill),
    PBIO_TEST(test_image_draw_image),
    PBIO_TEST(test_image_draw_pixel),
    PBIO_TEST(test_image_draw_line),
    PBIO_TEST(test_image_draw_thick_line),
    PBIO_TEST(test_image_draw_rect),
    PBIO_TEST(test_image_fill_rect),
    PBIO_TEST(test_image_draw_rounded_rect),
    PBIO_TEST(test_image_fill_rounded_rect),
    PBIO_TEST(test_image_draw_circle),
    PBIO_TEST(test_image_fill_circle),
    END_OF_TESTCASES
};
