// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_USB_SIMULATION

#include <pbdrv/usb.h>
#include "usb.h"

#include <pbio/error.h>
#include <pbio/os.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../rproc/rproc_virtual.h"

pbio_error_t pbdrv_usb_wait_until_configured(pbio_os_state_t *state) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

bool pbdrv_usb_is_ready(void) {
    return true;
}

pbdrv_usb_bcd_t pbdrv_usb_get_bcd(void) {
    return PBDRV_USB_BCD_NONE;
}

pbio_error_t pbdrv_usb_tx_chunk(pbio_os_state_t *state, const uint8_t *data, uint32_t size) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // The common driver hands us a COBS-encoded frame with a trailing
    // delimiter. This mock only forwards stdout to the native console, so
    // decode the frame and write out the payload of stdout events only.
    // REVISIT: This assumes that we do one chunk per stdout event. That is
    // currently true for the logic in usb.c, but we should revise this to make
    // it like the RX path if we turn it into an actual stream.
    uint8_t msg[PBDRV_USB_MAX_DECODED_MESSAGE_SIZE];
    uint32_t msg_size = pbdrv_usb_cobs_decode(data, size - 1, msg, sizeof(msg));

    if (msg_size >= 2 && msg[0] == PBIO_PYBRICKS_IN_EP_MSG_EVENT &&
        msg[1] == PBIO_PYBRICKS_EVENT_WRITE_STDOUT) {
        int ret = write(STDOUT_FILENO, &msg[2], msg_size - 2);
        (void)ret;

        #ifdef PBDRV_CONFIG_RPROC_VIRTUAL
        pbdrv_rproc_virtual_socket_send(&msg[2], msg_size - 2);
        #endif
    }

    // Simulate some I/O time.
    PBIO_OS_AWAIT_MS(state, &timer, 1);

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_usb_tx_reset(pbio_os_state_t *state) {
    return PBIO_SUCCESS;
}

static uint8_t usb_in_buf[PBDRV_USB_MAX_ENCODED_MESSAGE_SIZE];
static uint32_t usb_in_size;

uint32_t pbdrv_usb_get_data_and_start_receive(uint8_t *data) {

    // Invalid size.
    if (usb_in_size > sizeof(usb_in_buf)) {
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

// Simulates incoming USB data by reading the raw byte stream from native host
// stdin. In MicroPython, it drives the REPL.
static pbio_error_t pbdrv_usb_test_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // Fake a subscribe message so the common driver starts sending events,
    // just as a real host would after opening the port. It is COBS-encoded
    // like real host traffic.
    static const uint8_t subscribe_msg[] = {
        PBIO_PYBRICKS_OUT_EP_MSG_SUBSCRIBE, 1,
    };
    usb_in_size = pbdrv_usb_cobs_encode(subscribe_msg, sizeof(subscribe_msg), usb_in_buf);

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

        // Read raw bytes from native stdin and present them to the common
        // driver as a COBS-framed write stdin command, the same way a real
        // host would. This has been made non-blocking in platform.c.
        static uint8_t cmd[PBDRV_USB_MAX_DECODED_MESSAGE_SIZE];
        cmd[0] = PBIO_PYBRICKS_OUT_EP_MSG_COMMAND;
        cmd[1] = PBIO_PYBRICKS_COMMAND_WRITE_STDIN;
        ssize_t num_read = read(STDIN_FILENO, &cmd[2], sizeof(cmd) - 2);
        if (num_read > 0) {
            usb_in_size = pbdrv_usb_cobs_encode(cmd, 2 + num_read, usb_in_buf);
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_usb_init_device(void) {
    static pbio_os_process_t pbdrv_usb_test_process;
    pbio_os_process_start(&pbdrv_usb_test_process, pbdrv_usb_test_process_thread, NULL);

    // No physical port to open, so assert DTR right away. The process thread
    // also fakes a subscribe message, so the connection becomes active.
    pbdrv_usb_on_dtr_changed(true);
}

void pbdrv_usb_deinit_device(void) {
}

#endif // PBDRV_CONFIG_USB_SIMULATION
