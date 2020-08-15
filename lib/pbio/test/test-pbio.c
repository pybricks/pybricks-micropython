// SPDX-License-Identifier: MIT
// Copyright 2019-2020 The Pybricks Authors

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/main.h>

#include <contiki.h>

#include "src/processes.h"

#define PBIO_TEST_TIMEOUT 10000 // milliseconds

// Use these macros to define tests that _don't_ require a Contiki event loop
#define PBIO_TEST_FUNC(name) void name(void *env)
#define PBIO_TEST(name) \
    { #name, name, TT_FORK, NULL, NULL }

// Use these macros to define tests that _do_ require a Contiki event loop
#define PBIO_TP_THREAD_TEST_FUNC(name) PT_THREAD(name(struct pt *pt))
#define PBIO_PT_THREAD_TEST(name) \
    { #name, pbio_test_run_thread, TT_FORK, &pbio_test_setup, name }


static void pbio_test_run_thread(void *env) {
    char (*test_thread)(struct pt *pt) = env;
    struct pt pt;
    struct timer timer;

    pbio_init();
    process_start(&etimer_process, NULL);

    PT_INIT(&pt)
    timer_set(&timer, clock_from_msec(PBIO_TEST_TIMEOUT));

    while (PT_SCHEDULE(test_thread(&pt))) {
        pbio_do_one_event();
        if (timer_expired(&timer)) {
            tt_abort_printf(("Test timed out on line %d", pt.lc));
        }
    }

end:
    process_exit(&etimer_process);
    pbio_deinit();
}

static void *setup(const struct testcase_t *test_case) {
    // just passing through the protothread
    return test_case->setup_data;
}

static int cleanup(const struct testcase_t *test_case, void *env) {
    return 1;
}

static struct testcase_setup_t pbio_test_setup = {
    .setup_fn = setup,
    .cleanup_fn = cleanup,
};

// PBDRV

PBIO_TEST_FUNC(test_pwm_get);
PBIO_TEST_FUNC(test_pwm_set_duty);

static struct testcase_t pbdrv_pwm_tests[] = {
    PBIO_TEST(test_pwm_get),
    PBIO_TEST(test_pwm_set_duty),
    END_OF_TESTCASES
};

// PBIO

PBIO_TEST_FUNC(test_rgb_to_hsv);
PBIO_TEST_FUNC(test_hsv_to_rgb);
PBIO_TEST_FUNC(test_color_to_hsv);
PBIO_TEST_FUNC(test_color_to_rgb);

static struct testcase_t pbio_color_tests[] = {
    PBIO_TEST(test_rgb_to_hsv),
    PBIO_TEST(test_hsv_to_rgb),
    PBIO_TEST(test_color_to_hsv),
    PBIO_TEST(test_color_to_rgb),
    END_OF_TESTCASES
};

PBIO_TEST_FUNC(test_sqrt);
PBIO_TEST_FUNC(test_mul_i32_fix16);
PBIO_TEST_FUNC(test_div_i32_fix16);

static struct testcase_t pbio_math_tests[] = {
    PBIO_TEST(test_sqrt),
    PBIO_TEST(test_mul_i32_fix16),
    PBIO_TEST(test_div_i32_fix16),
    END_OF_TESTCASES
};

PBIO_TP_THREAD_TEST_FUNC(test_boost_color_distance_sensor);
PBIO_TP_THREAD_TEST_FUNC(test_boost_interactive_motor);
PBIO_TP_THREAD_TEST_FUNC(test_technic_large_motor);
PBIO_TP_THREAD_TEST_FUNC(test_technic_xl_motor);

static struct testcase_t pbio_uartdev_tests[] = {
    PBIO_PT_THREAD_TEST(test_boost_color_distance_sensor),
    PBIO_PT_THREAD_TEST(test_boost_interactive_motor),
    PBIO_PT_THREAD_TEST(test_technic_large_motor),
    PBIO_PT_THREAD_TEST(test_technic_xl_motor),
    END_OF_TESTCASES
};

// PBSYS

PBIO_TEST_FUNC(test_status);

static struct testcase_t pbsys_status_tests[] = {
    PBIO_TEST(test_status),
    END_OF_TESTCASES
};

static struct testgroup_t test_groups[] = {
    { "drv/pwm/", pbdrv_pwm_tests },
    { "src/color/", pbio_color_tests },
    { "src/math/", pbio_math_tests },
    { "src/uartdev/", pbio_uartdev_tests, },
    { "sys/status/", pbsys_status_tests, },
    END_OF_GROUPS
};

int main(int argc, const char **argv) {
    return tinytest_main(argc, argv, test_groups);
}
