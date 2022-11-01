// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <btstack.h>
#include <hci_dump_posix_stdout.h>
#include <tinytest.h>
#include <tinytest_macros.h>

#include <pbio/main.h>

#include <contiki.h>

#include "src/processes.h"

#define PBIO_TEST_TIMEOUT 1 // seconds

void pbio_test_run_thread(void *env) {
    PT_THREAD((*test_thread)(struct pt *pt)) = env;
    struct pt pt;
    struct timespec start_time, now_time;
    int timeout = PBIO_TEST_TIMEOUT;

    const char *pbio_test_timeout = getenv("PBIO_TEST_TIMEOUT");
    if (pbio_test_timeout) {
        timeout = atoi(pbio_test_timeout);
    }

    // REVISIT: we may also want to enable debug logging in non-thread tests
    int debug = 0;
    const char *pbio_test_debug = getenv("PBIO_TEST_DEBUG");
    if (pbio_test_debug) {
        debug = atoi(pbio_test_debug);
    }

    if (debug) {
        hci_dump_init(hci_dump_posix_stdout_get_instance());
    }
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_DEBUG, debug);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_INFO, debug);
    hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_ERROR, 1);

    pbio_init();

    PT_INIT(&pt);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (PT_SCHEDULE(test_thread(&pt))) {
        pbio_do_one_event();
        if (timeout > 0) {
            clock_gettime(CLOCK_MONOTONIC, &now_time);
            if (difftime(now_time.tv_sec, start_time.tv_sec) > timeout) {
                tt_abort_printf(("Test timed out on line %d", pt.lc));
            }
        }
    }

end:;
}

static void *setup(const struct testcase_t *test_case) {
    // just passing through the protothread
    return test_case->setup_data;
}

static int cleanup(const struct testcase_t *test_case, void *env) {
    return 1;
}

struct testcase_setup_t pbio_test_setup = {
    .setup_fn = setup,
    .cleanup_fn = cleanup,
};

extern struct testcase_t pbdrv_bluetooth_tests[];
extern struct testcase_t pbdrv_counter_tests[];
extern struct testcase_t pbdrv_pwm_tests[];
extern struct testcase_t pbio_battery_tests[];
extern struct testcase_t pbio_color_tests[];
extern struct testcase_t pbio_light_animation_tests[];
extern struct testcase_t pbio_color_light_tests[];
extern struct testcase_t pbio_light_matrix_tests[];
extern struct testcase_t pbio_math_tests[];
extern struct testcase_t pbio_task_tests[];
extern struct testcase_t pbio_trajectory_tests[];
extern struct testcase_t pbio_uartdev_tests[];
extern struct testcase_t pbio_util_tests[];
extern struct testcase_t pbsys_bluetooth_tests[];
extern struct testcase_t pbsys_status_tests[];
static struct testgroup_t test_groups[] = {
    { "drv/bluetooth/", pbdrv_bluetooth_tests },
    { "drv/counter/", pbdrv_counter_tests },
    { "drv/pwm/", pbdrv_pwm_tests },
    { "src/battery/", pbio_battery_tests },
    { "src/color/", pbio_color_tests },
    { "src/light/", pbio_light_animation_tests },
    { "src/light/", pbio_color_light_tests },
    { "src/light/", pbio_light_matrix_tests },
    { "src/math/", pbio_math_tests },
    { "src/task/", pbio_task_tests, },
    { "src/trajectory/", pbio_trajectory_tests },
    { "src/uartdev/", pbio_uartdev_tests, },
    { "src/util/", pbio_util_tests, },
    { "sys/bluetooth/", pbsys_bluetooth_tests, },
    { "sys/status/", pbsys_status_tests, },
    END_OF_GROUPS
};

int main(int argc, const char **argv) {
    const char *results_dir = getenv("PBIO_TEST_RESULTS_DIR");
    if (results_dir) {
        if (chdir(results_dir) == -1) {
            perror("chdir failed");
        }
    }

    return tinytest_main(argc, argv, test_groups);
}
