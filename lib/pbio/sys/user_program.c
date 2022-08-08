// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <stdbool.h>
#include <stddef.h>

#include <contiki.h>

#include <pbio/button.h>
#include <pbio/main.h>

#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>
#include "user_program.h"

// user program stop function
static pbsys_user_program_stop_t user_stop_func;

// Button combination that will trigger user program stop callback
static pbio_button_flags_t stop_buttons = PBIO_BUTTON_CENTER;
// State for button press one-shot
static bool stop_button_pressed;

/**
 * Prepares the runtime for running a user program.
 * @param [in]  callbacks   Optional struct of callback function pointers.
 */
void pbsys_user_program_prepare(const pbsys_user_program_callbacks_t *callbacks) {
    if (callbacks) {
        user_stop_func = callbacks->stop;
        pbsys_bluetooth_rx_set_callback(callbacks->stdin_event);
    } else {
        user_stop_func = NULL;
        pbsys_bluetooth_rx_set_callback(NULL);
    }
    pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);

    // Handle pending events triggered by the status change, e.g. start status
    // light animation.
    while (pbio_do_one_event()) {
    }
}

/**
 * Performs cleanup/reset after running a user program.
 */
void pbsys_user_program_unprepare(void) {
    pbsys_status_clear(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
    pbio_stop_all();
    user_stop_func = NULL;
    stop_buttons = PBIO_BUTTON_CENTER;
    pbsys_bluetooth_rx_set_callback(NULL);
}

/**
 * Request the user program to stop. For example, in MicroPython, this may raise
 * a SystemExit exception.
 */
void pbsys_user_program_stop(void) {
    if (user_stop_func) {
        user_stop_func();
    }
}

/**
 * Sets the stop button(s) to be used during a user program.
 * @param [in]  buttons     One or more button flags to set the stop button
 *                          or 0 to disable the stop button.
 */
void pbsys_user_program_set_stop_buttons(pbio_button_flags_t buttons) {
    stop_buttons = buttons;
}

/**
 * This is called periodically to monitor the user program.
 */
void pbsys_user_program_poll(void) {
    pbio_button_flags_t btn;
    pbio_button_is_pressed(&btn);

    if (!stop_buttons) {
        return;
    }

    if ((btn & stop_buttons) == stop_buttons) {
        if (!stop_button_pressed) {
            stop_button_pressed = true;
            pbsys_user_program_stop();
        }
    } else {
        stop_button_pressed = false;
    }
}

#if PBSYS_CONFIG_USER_PROGRAM_LOAD

PROCESS(pbsys_user_program_process, "user_program");

pbsys_user_program_info_t info;

/**
 * Certain size values indicate that no program will be sent.
 *
 * Instead, handle the special case directly so we can exit the process.
 */
static void pbsys_user_program_set_no_program(uint32_t size) {

    // Check if it is a special size.
    if (size == PSYS_USER_PROGRAM_TYPE_BUILTIN_0) {
        info.program_type = size;
    }
    // Otherwise we are dealing with an invalid program.
    else {
        info.program_type = PSYS_USER_PROGRAM_TYPE_NONE;
    }

    // We don't want to override any stored potentially stored programs, so the
    // heap starts after user data.
    info.heap_start = info.program_data + info.program_size;
}

/**
 * Sets program size after successful reception.
 */
static void pbsys_user_program_set_size(uint32_t size) {
    if (size > info.program_size_max) {
        pbsys_user_program_set_no_program(size);
        return;
    }

    // Set system resources based on received program size.
    info.program_size = size;
    info.heap_start = info.program_data + size;
    info.program_type = PSYS_USER_PROGRAM_TYPE_NORMAL;
}

void pbsys_user_program_process_start(pbsys_user_program_info_t **program_info) {

    // Set application heap information.
    extern uint32_t _heap_start;
    extern uint32_t _heap_end;
    info.heap_end = (uint8_t *)&_heap_end;
    info.program_data = (uint8_t *)&_heap_start;
    info.program_size_max = info.heap_end - info.program_data;

    // Set application stack information.
    extern uint32_t _estack;
    extern uint32_t _sstack;
    info.stack_start = (uint8_t *)&_sstack;
    info.stack_end = (uint8_t *)&_estack;

    // Start the process.
    process_start(&pbsys_user_program_process);

    // Return reference to program info.
    *program_info = &info;
}

bool pbsys_user_program_process_complete(void) {

    // Stop receiving the program on shutdown.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        process_exit(&pbsys_user_program_process);
        pbsys_user_program_set_no_program(PSYS_USER_PROGRAM_TYPE_NONE);
        return true;
    }

    // Check if the process is complete.
    if (!process_is_running(&pbsys_user_program_process)) {
        return true;
    }

    // REVISIT: Explicit polling should not be needed if we
    // use the bluetooth callbacks to get events posted.
    process_poll(&pbsys_user_program_process);
    return false;
}

PROCESS_THREAD(pbsys_user_program_process, ev, data) {

    static pbio_error_t err;
    static pbio_button_flags_t btn;

    // Checksum and size of current message.
    static uint8_t checksum;
    static uint32_t rtx_size;

    // Buffers representing size of program to be received
    // and the current chunk of the program.
    static uint8_t size_buf[4];
    static uint8_t *chunk_buf;

    // Total expected program size, progress size, and current chunk size
    static uint32_t expected_size;
    static uint32_t remaining_size;
    static uint32_t chunk_size;
    static uint32_t remaining_chunk_size;

    static struct etimer timer;

    PROCESS_BEGIN();

    // Reset program type in case reception fails halfway.
    pbsys_user_program_set_no_program(PSYS_USER_PROGRAM_TYPE_NONE);

    // Make sure button is released. Otherwise wait.
    PROCESS_WAIT_WHILE(pbio_button_is_pressed(&btn) == PBIO_SUCCESS && (btn & PBIO_BUTTON_CENTER));

    // Flush any buffered bytes from stdin.
    while (pbsys_bluetooth_rx_get_available()) {
        PROCESS_WAIT_UNTIL(({
            rtx_size = 1;
            pbsys_bluetooth_rx(size_buf, &rtx_size) == PBIO_SUCCESS;
        }));
    }

    // Receive program size or cancel on button press.
    PROCESS_WAIT_UNTIL(({
        rtx_size = 4;
        (
            // Stop waiting if button pressed.
            (pbio_button_is_pressed(&btn) == PBIO_SUCCESS &&
                (btn & PBIO_BUTTON_CENTER) &&
                (err = PBIO_ERROR_CANCELED))
            ||
            // Or stop waiting if full size message received.
            ((err = pbsys_bluetooth_rx(size_buf, &rtx_size)) == PBIO_SUCCESS)
        );
    }));

    // Exit on button press. Stop receiving and run internal program.
    if (err == PBIO_ERROR_CANCELED) {
        // Wait for button release.
        PROCESS_WAIT_WHILE(pbio_button_is_pressed(&btn) == PBIO_SUCCESS && (btn & PBIO_BUTTON_CENTER));

        // Use existing program, so set type to run without changing data.
        pbsys_user_program_set_size(info.program_size);
        PROCESS_EXIT();
    }

    // Handle invalid programs.
    expected_size = pbio_get_uint32_le(size_buf);
    if (err != PBIO_SUCCESS || expected_size > info.program_size_max) {
        pbsys_user_program_set_no_program(expected_size);
        PROCESS_EXIT();
    }

    // Respond with checksum to confirm valid program size.
    checksum = pbio_util_get_checksum(PBIO_UTIL_CHECKSUM_TYPE_XOR8_START_00, size_buf, sizeof(size_buf));
    PROCESS_WAIT_UNTIL(({
        rtx_size = 1;
        pbsys_bluetooth_tx(&checksum, &rtx_size) == PBIO_SUCCESS;
    }));

    // Receive program chunk by chunk.
    remaining_size = expected_size;
    while (remaining_size) {

        etimer_set(&timer, 500);

        // Size of chunk to receive now, and location of that chunk.
        chunk_size = remaining_size < PBSYS_CONFIG_DOWNLOAD_CHUNK_SIZE ?
            remaining_size : PBSYS_CONFIG_DOWNLOAD_CHUNK_SIZE;
        chunk_buf = info.program_data + expected_size - remaining_size;

        // Receive a chunk of data or stop on timeout. The chunk may be further
        // broken down by the sender, so receive it piece by piece.
        etimer_set(&timer, 500);
        remaining_chunk_size = chunk_size;
        while (remaining_chunk_size) {
            PROCESS_WAIT_UNTIL(({
                rtx_size = remaining_chunk_size;
                (
                    // Stop waiting on timeout.
                    etimer_expired(&timer) ||
                    // Or stop waiting if part of the chunk was received.
                    ((err = pbsys_bluetooth_rx(chunk_buf + chunk_size - remaining_chunk_size, &rtx_size)) == PBIO_SUCCESS)
                );
            }));
            remaining_chunk_size -= rtx_size;
        }

        // On timeout, exit process.
        if (etimer_expired(&timer)) {
            PROCESS_EXIT();
        }

        // Respond with checksum to confirm valid program chunk.
        etimer_set(&timer, 100);
        checksum = pbio_util_get_checksum(PBIO_UTIL_CHECKSUM_TYPE_XOR8_START_00, chunk_buf, chunk_size);
        PROCESS_WAIT_UNTIL(({
            rtx_size = 1;
            (etimer_expired(&timer) || pbsys_bluetooth_tx(&checksum, &rtx_size) == PBIO_SUCCESS);
        }));

        // On timeout, exit process.
        if (etimer_expired(&timer)) {
            PROCESS_EXIT();
        }

        // Reduce remaining size by received chunk size.
        remaining_size -= chunk_size;
    }

    // On success, prepare program state for running.
    pbsys_user_program_set_size(expected_size);

    PROCESS_END();
}

#endif // PBSYS_CONFIG_USER_PROGRAM_LOAD
