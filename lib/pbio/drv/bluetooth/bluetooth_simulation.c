// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLUETOOTH_SIMULATION

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bluetooth.h"
#include <pbdrv/bluetooth.h>

#include <pbio/error.h>
#include <pbio/os.h>


char pbdrv_bluetooth_hub_name[16] = "Pybricks Hub";

pbio_error_t pbdrv_bluetooth_start_advertising_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_advertising_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

bool pbdrv_bluetooth_is_connected(pbdrv_bluetooth_connection_t connection) {
    if (connection == PBDRV_BLUETOOTH_CONNECTION_LE) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PYBRICKS) {
        return true;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_UART) {
        return false;
    }

    if (connection == PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL) {
        return false;
    }

    return false;
}


pbio_error_t pbdrv_bluetooth_send_pybricks_value_notification(pbio_os_state_t *state, const uint8_t *data, uint16_t size) {
    PBIO_OS_ASYNC_BEGIN(state);

    // Only care about stdout for now.
    if (size < 1 || data[0] != PBIO_PYBRICKS_EVENT_WRITE_STDOUT) {
        return PBIO_SUCCESS;
    }

    int ret = write(STDOUT_FILENO, data + 1, size - 1);
    (void)ret;

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_scan_and_connect_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_discover_characteristic_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_read_characteristic_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_write_characteristic_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_peripheral_disconnect_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_broadcasting_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_start_observing_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_stop_observing_func(pbio_os_state_t *state, void *context) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

const char *pbdrv_bluetooth_get_hub_name(void) {
    return pbdrv_bluetooth_hub_name;
}

const char *pbdrv_bluetooth_get_fw_version(void) {
    return "N/A";
}

void pbdrv_bluetooth_controller_reset_hard(void) {
}

pbio_error_t pbdrv_bluetooth_controller_reset(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_bluetooth_controller_initialize(pbio_os_state_t *state, pbio_os_timer_t *timer) {
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#define STDIN_HEADER_SIZE (1)

static void pbdrv_bluetooth_simulation_tick_handler() {
    #ifdef PBDRV_CONFIG_RUN_ON_CI
    // CI and MicroPython test suite have lots of problems with stdin. It is
    // only needed for the REPL and interactive input, so don't bother on CI.
    return;
    #endif

    uint8_t buf[256 + STDIN_HEADER_SIZE];

    // This has been made non-blocking in platform.c.
    ssize_t r = read(STDIN_FILENO, buf + STDIN_HEADER_SIZE, sizeof(buf) - STDIN_HEADER_SIZE);

    if (r > 0) {
        buf[0] = PBIO_PYBRICKS_COMMAND_WRITE_STDIN;
        pbdrv_bluetooth_receive_handler(buf, r + STDIN_HEADER_SIZE);
    } else if (r == 0) {
        // EOF.
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        // No data available.
    }
}

static pbio_os_process_t pbdrv_bluetooth_simulation_process;

static pbio_os_state_t bluetooth_thread_state;
static pbio_os_state_t bluetooth_thread_err;

/**
 * Placeholder process that does not have any state.
 */
static pbio_error_t pbdrv_bluetooth_simulation_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t simulation_timer = {
        .duration = 1,
    };

    if (pbio_os_timer_is_expired(&simulation_timer)) {
        pbio_os_timer_extend(&simulation_timer);

        pbdrv_bluetooth_simulation_tick_handler();
    }

    if (bluetooth_thread_err == PBIO_ERROR_AGAIN) {
        bluetooth_thread_err = pbdrv_bluetooth_process_thread(&bluetooth_thread_state, NULL);
    }
    return bluetooth_thread_err;
}

void pbdrv_bluetooth_init_hci(void) {
    bluetooth_thread_err = PBIO_ERROR_AGAIN;
    bluetooth_thread_state = 0;
    pbio_os_process_start(&pbdrv_bluetooth_simulation_process, pbdrv_bluetooth_simulation_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLUETOOTH_SIMULATION
