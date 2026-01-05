// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_SIMULATION

#include <pbdrv/usb.h>
#include "usb.h"

#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/protocol.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pbio_error_t pbdrv_usb_wait_for_charger(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

bool pbdrv_usb_is_ready(void) {
    return true;
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

pbio_error_t pbdrv_usb_tx_event(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);


    // Stdout also goes to native stdout.
    if (size > 2 && data[0] == PBIO_PYBRICKS_IN_EP_MSG_EVENT && data[1] == PBIO_PYBRICKS_EVENT_WRITE_STDOUT) {
        int ret = write(STDOUT_FILENO, data + 2, size - 2);
        (void)ret;
    }

    #ifndef PBDRV_CONFIG_RUN_ON_CI
    extern void virtual_hub_socket_send(const uint8_t *data, uint32_t size);
    virtual_hub_socket_send(data + 1, size - 1);
    #endif

    // Simulate some I/O time.
    PBIO_OS_AWAIT_MS(state, &timer, 1);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_response(pbio_os_state_t *state, pbio_pybricks_error_t code) {

    static pbio_os_timer_t timer;

    static uint8_t response_buf[1 + sizeof(uint32_t)] __attribute__((aligned(4))) =
    { PBIO_PYBRICKS_IN_EP_MSG_RESPONSE };

    PBIO_OS_ASYNC_BEGIN(state);

    // Response is just the error code.
    pbio_set_uint32_le(&response_buf[1], code);

    // Simulation never actually sends this.

    // Simulate some I/O time.
    PBIO_OS_AWAIT_MS(state, &timer, 2);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    return PBIO_SUCCESS;
}

static uint8_t usb_in_buf[PBDRV_CONFIG_USB_MAX_PACKET_SIZE];
static uint32_t usb_in_size;

uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {

    // Invalid size.
    if (usb_in_size > PBDRV_CONFIG_USB_MAX_PACKET_SIZE) {
        usb_in_size = 0;
    }

    // Nothing received yet.
    if (usb_in_size == 0) {
        return 0;
    }

    uint32_t size = usb_in_size;
    memcpy(data, usb_in_buf, size);

    // Reset to indicate we wait for new data.
    usb_in_size = 0;

    return size;
}

// Simulates incoming USB data by reading from native host stdin. In
// MicroPython, it drives the REPL.
static pbio_error_t pbdrv_usb_test_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // Simulate subscribe event.
    usb_in_buf[0] = PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE;
    usb_in_buf[1] = 1;
    usb_in_size = 2;

    #ifdef PBDRV_CONFIG_RUN_ON_CI
    // CI and MicroPython test suite have lots of problems with stdin. It is
    // only needed for the REPL and interactive input, so don't bother on CI.
    return PBIO_SUCCESS;
    #endif

    for (;;) {

        PBIO_OS_AWAIT_MS(state, &timer, 1);
        if (usb_in_size) {
            // Data not read yet.
            continue;
        }

        // This has been made non-blocking in platform.c.
        ssize_t num_read = read(STDIN_FILENO, &usb_in_buf[2], sizeof(usb_in_buf) - 2);
        if (num_read > 0) {
            usb_in_buf[0] = PBIO_PYBRICKS_OUT_EP_MSG_COMMAND;
            usb_in_buf[1] = PBIO_PYBRICKS_COMMAND_WRITE_STDIN;
            usb_in_size = 2 + num_read;
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_usb_init_device(void) {
    static pbio_os_process_t pbdrv_usb_test_process;
    pbio_os_process_start(&pbdrv_usb_test_process, pbdrv_usb_test_process_thread, NULL);
}

void pbdrv_usb_deinit_device(void) {
}

#endif // PBDRV_CONFIG_USB_SIMULATION
