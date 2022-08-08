// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stdint.h>

#include <pbdrv/clock.h>
#include <pbdrv/reset.h>

#include <pbio/main.h>

#include <pbsys/bluetooth.h>
#include <pbsys/main.h>
#include <pbsys/status.h>
#include <pbsys/user_program.h>

extern uint32_t _heap_start;
extern uint32_t _heap_end;

static uint8_t *program_data = (uint8_t *)&_heap_start;
static uint32_t program_size = 0;


void poll(void) {
    while (pbio_do_one_event()) {

    }
}

static pbio_error_t wait_for_button_release(void) {
    pbio_error_t err;
    pbio_button_flags_t btn = PBIO_BUTTON_CENTER;
    while (btn & PBIO_BUTTON_CENTER) {
        err = pbio_button_is_pressed(&btn);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        poll();
    }
    return PBIO_SUCCESS;
}

// Receive single character
int stdin_rx_chr(void) {
    uint32_t size;
    uint8_t c;

    // wait for rx interrupt
    while (size = 1, pbsys_bluetooth_rx(&c, &size) != PBIO_SUCCESS) {
        poll();
    }

    return c;
}

// Send string of given length
void stdout_tx_strn(const char *str, uint32_t len) {
    while (len) {
        uint32_t size = len;
        pbio_error_t err = pbsys_bluetooth_tx((const uint8_t *)str, &size);

        if (err == PBIO_SUCCESS) {
            str += size;
            len -= size;
            continue;
        }

        if (err != PBIO_ERROR_AGAIN) {
            // Ignoring error for now. This means stdout lost if Bluetooth is
            // disconnected.
            return;
        }

        poll();
    }
}

// Wait for data from an IDE
static pbio_error_t get_message(uint8_t *buf, uint32_t rx_len, int time_out) {
    // Maximum time between two bytes/chunks
    const uint32_t time_interval = 500;

    // Acknowledge at the end of each message or each data chunk
    const uint32_t chunk_size = 100;

    pbio_error_t err;

    // Initialize
    uint8_t checksum = 0;
    uint32_t rx_count = 0;
    uint32_t time_start = pbdrv_clock_get_ms();
    uint32_t time_now;
    pbio_button_flags_t btn;

    while (true) {

        // Check if button is pressed
        err = pbio_button_is_pressed(&btn);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        if (btn & PBIO_BUTTON_CENTER) {
            // If so, wait for release
            err = wait_for_button_release();
            if (err != PBIO_SUCCESS) {
                return err;
            }
            // Cancel waiting for message
            return PBIO_ERROR_CANCELED;
        }

        // Current time
        time_now = pbdrv_clock_get_ms();

        // Try to get one byte
        if (pbsys_bluetooth_rx_get_available()) {
            buf[rx_count] = stdin_rx_chr();
            // On success, reset timeout
            time_start = time_now;

            // Update checksum
            checksum ^= buf[rx_count];

            // Increment rx counters
            rx_count++;

            // When done, acknowledge with the checksum
            if (rx_count == rx_len) {
                stdout_tx_strn((const char *)&checksum, 1);
                return PBIO_SUCCESS;
            }

            // Acknowledge after receiving a chunk.
            if (rx_count % chunk_size == 0) {
                stdout_tx_strn((const char *)&checksum, 1);
                // Reset the checksum
                checksum = 0;
            }
        }

        // Check if we have timed out
        if (rx_count == 0) {
            // Use given timeout for first byte
            if (time_out >= 0 && time_now - time_start > (uint32_t)time_out) {
                return PBIO_ERROR_TIMEDOUT;
            }
        } else if (time_now - time_start > time_interval) {
            // After the first byte, apply much shorter interval timeout
            return PBIO_ERROR_TIMEDOUT;
        }

        // Keep polling
        poll();
    }
}

static uint32_t program_get_new_size(void) {
    // flush any buffered bytes from stdin
    while (pbsys_bluetooth_rx_get_available()) {
        stdin_rx_chr();
    }

    // Wait for program size message.
    uint32_t len;
    pbio_error_t err;
    err = get_message((uint8_t *)&len, sizeof(len), -1);

    // Did not get valid message or the button was pressed.
    // Return 0 to indicate we won't be getting a new program.
    if (err != PBIO_SUCCESS) {
        return 0;
    }

    // Return expected program length.
    return len;
}

static uint32_t user_program_receive(void) {

    const uint32_t program_size_max = ((char *)&_heap_end - (char *)&_heap_start);

    // Get expected size of program.
    uint32_t size = program_get_new_size();

    // Special program.
    if (size == 0x20202020) {
        return size;
    }

    // If the size is zero and there is already a program, run that.
    if (size == 0 && program_size > 0) {
        return program_size;
    }

    // Invalid program.
    if (size == 0 || size > program_size_max) {
        program_size = 0;
        return 0;
    }

    // Get the new program if the size is valid.
    pbio_error_t err = get_message(program_data, size, 500);
    if (err != PBIO_SUCCESS) {
        program_size = 0;
        return 0;
    }

    // Valid program, so return that.
    program_size = size;
    return size;
}

static void *pbsys_main_jmp_buf[5];

static void pb_sys_main_check_for_shutdown(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        pbio_set_event_hook(NULL);
        __builtin_longjmp(pbsys_main_jmp_buf, 1);
    }
}

/**
 * Initializes the PBIO library and runs custom main program.
 *
 * The main program may be abruptly ended when shutting down the hub.
 *
 * @param [in]  main    The main program.
 */
void pbsys_main(pbsys_main_t main) {
    pbio_init();

    // REVISIT: __builtin_setjmp() only saves a couple registers, so using it
    // could cause problems if we add more to this function. However, since we
    // use -nostdlib compile flag, we don't have setjmp(). We should be safe
    // for now though since we don't use any local variables after the longjmp.
    if (__builtin_setjmp(pbsys_main_jmp_buf) == 0) {
        // REVISIT: we could save a few CPU cycles on each call to pbio_do_one_event()
        // if we don't set this until shutdown is actually requested
        pbio_set_event_hook(pb_sys_main_check_for_shutdown);
        for (;;) {
            wait_for_button_release();
            main(program_data, user_program_receive());
        }
    } else {
        // in case we jumped out of the middle of a user program
        pbsys_user_program_unprepare();
    }

    // The power could be held on due to someone pressing the center button
    // or USB being plugged in, so we have this loop to keep pumping events
    // to turn off most of the peripherals and keep the battery charger running.
    for (;;) {
        // We must handle all pending events before turning the power off the
        // first time, otherwise the city hub turns itself back on sometimes.
        while (pbio_do_one_event()) {
        }
        pbdrv_reset_power_off();
    }
}
