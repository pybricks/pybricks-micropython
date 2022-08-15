// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_PROGRAM_LOAD

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <contiki.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/math.h>
#include <pbio/protocol.h>

#include "core.h"

#include <pbsys/bluetooth.h>
#include <pbsys/main.h>
#include <pbsys/status.h>

#include <pbdrv/block_device.h>

/**
 * Map of loaded data. All data types are little-endian.
 */
typedef struct {
    /**
     * How much to write on shutdown. This is reset to 0 on load, and should be
     * set whenever any data is updated. This must always remain the first
     * element of this structure.
     */
    uint32_t write_size;
    /**
     * Size of the application program.
     */
    uint32_t program_size;
    /**
     * Data of the application program.
     */
    uint8_t program_data[];
} data_map_t;

// The data map sits at the start of user RAM.
extern uint32_t _pbsys_program_load_user_ram_start;
static data_map_t *map = (data_map_t *)&_pbsys_program_load_user_ram_start;

// Gets the (constant) maximum program size.
static inline uint32_t pbsys_program_load_get_max_program_size() {
    return pbdrv_block_device_get_size() - sizeof(data_map_t);
}

// Updates the current program size.
static inline void pbsys_program_load_set_program_size(uint32_t size) {
    map->program_size = size;
    // Data was updated, so set the write size.
    map->write_size = size + sizeof(data_map_t);
}

PROCESS(pbsys_program_load_process, "program_load");

/**
 * Starts loading the user data from storage to RAM.
 */
void pbsys_program_load_init(void) {
    pbsys_init_busy_up();
    process_start(&pbsys_program_load_process);
}

/**
 * Starts saving the user data from RAM to storage.
 */
void pbsys_program_load_deinit(void) {
    pbsys_init_busy_up();
    process_post(&pbsys_program_load_process, PROCESS_EVENT_CONTINUE, NULL);
}

/**
 * Polls the program load process, used by async block device functions.
 */
static void pbsys_program_load_process_callback(void) {
    process_poll(&pbsys_program_load_process);
}

/**
 * This process loads data from storage on boot, and saves it on shutdown.
 */
PROCESS_THREAD(pbsys_program_load_process, ev, data) {

    static pbio_error_t err;
    static struct pt pt;

    PROCESS_BEGIN();

    // Make the block device async functions poll this process when there
    // is an event to process.
    pbdrv_block_device_set_callback(pbsys_program_load_process_callback);

    // Read size of stored data.
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, sizeof(map->write_size), &err));

    // Read the available data into RAM.
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, map->write_size, &err));
    if (err != PBIO_SUCCESS) {
        map->program_size = 0;
    }

    // Reset write size, so we don't write data if nothing changed.
    map->write_size = 0;

    // Initialization done.
    pbsys_init_busy_down();

    // Wait for signal on signal.
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);

    // Write data to storage if it was updated.
    if (map->write_size) {
        PROCESS_PT_SPAWN(&pt, pbdrv_block_device_store(&pt, (uint8_t *)map, map->write_size, &err));
    }

    pbdrv_block_device_set_callback(NULL);

    // Deinitialization done.
    pbsys_init_busy_down();

    PROCESS_END();
}

static PT_THREAD(pbsys_program_receive_chunk(struct pt *pt, uint8_t *data, uint32_t size, pbio_error_t *err)) {

    static struct timer timer;
    static uint32_t remaining;
    static uint32_t xfer_size;
    uint8_t checksum;

    PT_BEGIN(pt);

    // Sender may send smaller chunks, so receive it piece by piece.
    remaining = size;
    while (remaining) {

        // Wait for any number of bytes.
        timer_set(&timer, 500);
        PT_WAIT_UNTIL(pt, ({
            xfer_size = remaining;
            (
                // Stop waiting if data is available.
                ((*err = pbsys_bluetooth_rx(data + size - remaining, &xfer_size)) != PBIO_ERROR_AGAIN) ||
                // Stop waiting on timeout.
                (timer_expired(&timer) && (*err = PBIO_ERROR_TIMEDOUT))
            );
        }));

        // Exit on error.
        if (*err != PBIO_SUCCESS) {
            PT_EXIT(pt);
        }

        // On sucess, decrement remaining chunk size.
        remaining -= xfer_size;
    }

    // Get checksum.
    checksum = 0;
    for (uint32_t i = 0; i < size; i++) {
        checksum ^= data[i];
    }

    // Respond with checksum.
    timer_set(&timer, 100);
    PT_WAIT_UNTIL(pt, ({
        xfer_size = 1;
        (
            ((*err = pbsys_bluetooth_tx(&checksum, &xfer_size)) != PBIO_ERROR_AGAIN) ||
            (timer_expired(&timer) && (*err = PBIO_ERROR_TIMEDOUT))
        );
    }));

    PT_END(pt);
}

static bool button_is_pressed(void) {
    pbio_button_flags_t btn;
    return pbio_button_is_pressed(&btn) == PBIO_SUCCESS && (btn & PBIO_BUTTON_CENTER);
}

static PT_THREAD(pbsys_program_receive_thread(struct pt *pt, pbio_error_t *err, uint32_t *received_size)) {

    static pbio_button_flags_t btn;

    // Buffer for incoming program size.
    static uint8_t size_buf[4];
    static uint32_t incoming_size;
    static uint32_t remaining_size;

    // Buffer for data chunk.
    static uint32_t chunk_size;
    static uint8_t *chunk_buf;

    // Child protothread for receiving chunk of data.
    static struct pt child;

    PT_BEGIN(pt);

    // The first time after boot, the Prime Hub buttons are not ready.
    // REVISIT: We should be waiting on this in pbio init, not here.
    PT_WAIT_UNTIL(pt, pbio_button_is_pressed(&btn) == PBIO_SUCCESS);

    // Make sure button is released. Otherwise wait.
    PT_WAIT_WHILE(pt, button_is_pressed());

    // Flush any buffered bytes from stdin.
    pbsys_bluetooth_rx_flush();

    // Wait for incoming data or button press.
    PT_WAIT_UNTIL(pt, button_is_pressed() || pbsys_bluetooth_rx_get_available());

    // Stop receiving on button press.
    if (!pbsys_bluetooth_rx_get_available()) {
        // Wait for button release.
        PT_WAIT_WHILE(pt, button_is_pressed());
        // No size received.
        *received_size = 0;
        // This ends the thread successfully.
        *err = PBIO_SUCCESS;
        PT_EXIT(pt);
    }

    // Receive program size.
    PT_SPAWN(pt, &child, pbsys_program_receive_chunk(&child, size_buf, sizeof(size_buf), err));
    if (*err != PBIO_SUCCESS) {
        PT_EXIT(pt);
    }

    // Handle sizes too big to receive.
    incoming_size = pbio_get_uint32_le(size_buf);
    if (incoming_size > pbsys_program_load_get_max_program_size()) {
        // We won't be receiving a program this big but we may still want to
        // respond to this size command, so exit successfully.
        *received_size = incoming_size;
        *err = PBIO_SUCCESS;
        PT_EXIT(pt);
    }

    // Reset stored size since data is garbage if reception fails.
    pbsys_program_load_set_program_size(0);

    // Receive program chunk by chunk.
    remaining_size = incoming_size;
    while (remaining_size) {

        // Size and buffer for the current chunk.
        chunk_size = pbio_math_min(remaining_size, PBIO_PYBRICKS_PROTOCOL_DOWNLOAD_CHUNK_SIZE);
        chunk_buf = map->program_data + incoming_size - remaining_size;

        // Receive the chunk.
        PT_SPAWN(pt, &child, pbsys_program_receive_chunk(&child, chunk_buf, chunk_size, err));
        if (*err != PBIO_SUCCESS) {
            PT_EXIT(pt);
        }
        remaining_size -= chunk_size;
    }

    // On success, prepare program state for running.
    pbsys_program_load_set_program_size(incoming_size);
    *received_size = incoming_size;

    PT_END(pt);
}

/**
 * Receives or loads a program. This will make the hub wait for either:
 *
 *   - A button press to start running the stored program.
 *   - A special program size to start running a builtin application program.
 *   - A size and program payload to store and then start running.
 *
 * @param [in]  program         Program info structure to be populated.
 * @return                      ::PBIO_SUCCESS on success.
 *                              ::PBIO_ERROR_CANCELED when canceled due to shutdown.
 *                              ::PBIO_ERROR_TIMEDOUT if receiving data chunk timed out.
 *                              ::PBIO_ERROR_INVALID_ARG if the received size was too big.
 */
pbio_error_t pbsys_program_load_receive(pbsys_main_program_t *program) {

    static struct pt pt;
    static pbio_error_t err;

    // Keep track of received size in case it isn't actually a size but a
    // command encoded as a size to trigger a builtin program.
    static uint32_t received_size;

    // Run the receive protothread until completion or shutdown.
    PT_INIT(&pt);
    while (PT_SCHEDULE(pbsys_program_receive_thread(&pt, &err, &received_size))) {
        pbio_do_one_event();
        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
            return PBIO_ERROR_CANCELED;
        }
    }

    // Handle error from receive thread.
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get application data.
    program->size = map->program_size;
    program->data = map->program_data;
    program->run_builtin = false;

    // This special size is used to indicate running a builtin program.
    if (received_size == 0x20202020) {
        program->run_builtin = true;
        return PBIO_SUCCESS;
    }

    // Don't run invalid programs.
    if (map->program_size == 0 || received_size > pbsys_program_load_get_max_program_size()) {
        // TODO: Validate the data beyond just size.
        return PBIO_ERROR_INVALID_ARG;
    }

    // All checks have passed, so we can run this program.
    return PBIO_SUCCESS;
}

#endif // PBSYS_CONFIG_PROGRAM_LOAD
