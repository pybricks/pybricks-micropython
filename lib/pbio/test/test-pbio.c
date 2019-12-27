// SPDX-License-Identifier: MIT
// Copyright 2019 David Lechner <david@pybricks.com>

#include <stdio.h>

#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/main.h>

#include <contiki.h>

#include "src/processes.h"

#define PBIO_TEST_TIMEOUT 10000 // milliseconds

// Use this macro to define tests that _don't_ require a Contiki event loop
#define PBIO_TEST(name) \
    { #name, name, TT_FORK, NULL, NULL }

// Use this macro to define tests that _do_ require a Contiki event loop
#define PBIO_PT_THREAD_TEST(name) \
    { #name, pbio_test_run_thread, TT_FORK, &pbio_test_setup, name }

#define PBIO_TEST_FUNC(name) void name(void *env)


// TODO: this can go away once we have real tests that use pbio_test_run_thread()
static PT_THREAD(example_test(struct pt *pt)) {
    PT_BEGIN(pt);

    PT_END(pt);
}


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

static struct testcase_t example_tests[]  = {
    PBIO_PT_THREAD_TEST(example_test),
    END_OF_TESTCASES
};

PBIO_TEST_FUNC(pbio_math_test_sqrt);

static struct testcase_t pbio_math_tests[] = {
    PBIO_TEST(pbio_math_test_sqrt),
    END_OF_TESTCASES
};

PBIO_TEST_FUNC(test_boost_color_distance_sensor);
PBIO_TEST_FUNC(test_boost_interactive_motor);
PBIO_TEST_FUNC(test_technic_large_motor);
PBIO_TEST_FUNC(test_technic_xl_motor);

static struct testcase_t pbio_uartdev_tests[] = {
    PBIO_PT_THREAD_TEST(test_boost_color_distance_sensor),
    PBIO_PT_THREAD_TEST(test_boost_interactive_motor),
    PBIO_PT_THREAD_TEST(test_technic_large_motor),
    PBIO_PT_THREAD_TEST(test_technic_xl_motor),
    END_OF_TESTCASES
};

static struct testgroup_t test_groups[] = {
    { "example/", example_tests },
    { "math/", pbio_math_tests },
    { "uartdev/", pbio_uartdev_tests, },
    END_OF_GROUPS
};

int main(int argc, const char **argv) {
    return tinytest_main(argc, argv, test_groups);
}
