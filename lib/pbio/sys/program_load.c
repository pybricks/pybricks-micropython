// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_PROGRAM_LOAD

#include <stdbool.h>
#include <stddef.h>

#include <contiki.h>

#include <pbio/button.h>
#include <pbio/main.h>

#include <pbsys/bluetooth.h>
#include <pbsys/status.h>
#include <pbsys/program_load.h>

#include "main.h"
#include "program_load.h"

extern uint32_t _app_data_ram_start;
extern uint32_t _heap_end;
extern uint32_t _estack;
extern uint32_t _sstack;

/**
 * Map of stored data. Loaded into RAM on boot, and written back on shutdown
 * if write_size is nonzero.
 */
typedef struct {
    /**
     * Total data size of this structure, including variable-sized data at the
     * end. Should only be set if we should write on shutdown, i.e. if any
     * data was updated.
     */
    uint32_t write_size;
    /**
     * Checksum of everything after the checksum. REVISIT: Actually implement it.
     */
    uint32_t checksum;
    /**
     * Persistent storage for end-user variables or settings.
     */
    uint8_t user_data[PBSYS_CONFIG_PROGRAM_USER_DATA_SIZE];
    /**
     * Size of the application program.
     */
    uint32_t program_size;
    /**
     * Data of the application program.
     */
    uint8_t program_data[];
} __attribute__((packed, scalar_storage_order("little-endian"))) data_map_t;

static data_map_t *map = (data_map_t *)&_app_data_ram_start;

/**
 * Updates stored size of a program.
 */
static void set_stored_program_size(uint32_t size) {
    // Update program size.
    map->program_size = size;

    // Update total size, which is also used as an indicator that data
    // will be saved on shutdown.
    map->write_size = size + sizeof(data_map_t);
}

#define PROGRAM_SIZE_MAX ((uint8_t *)&_heap_end - (uint8_t *)&_app_data_ram_start - sizeof(data_map_t))

PROCESS(pbsys_program_receive_process, "program_receive");

/**
 * Holds the received size result of pbsys_program_receive_process.
 */
static uint32_t last_received_size;

PROCESS_THREAD(pbsys_program_receive_process, ev, data) {

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
    static uint32_t incoming_size;
    static uint32_t remaining_size;
    static uint32_t chunk_size;
    static uint32_t remaining_chunk_size;

    static struct etimer timer;

    PROCESS_BEGIN();

    // Reset received size in case reception fails halfway.
    last_received_size = PSYS_PROGRAM_LOAD_TYPE_NONE;

    // The first time after boot, the buttons are not ready.
    // REVISIT: We should be waiting on this in pbio init, not here.
    PROCESS_WAIT_UNTIL(pbio_button_is_pressed(&btn) == PBIO_SUCCESS);

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

        // Did not receive a size.
        last_received_size = 0;

        // Exit the process without any size update.
        PROCESS_EXIT();
    }

    // Handle sizes too big to receive.
    incoming_size = pbio_get_uint32_le(size_buf);
    if (err != PBIO_SUCCESS || incoming_size > PROGRAM_SIZE_MAX) {
        // Store size in case it means we have to do something special.
        last_received_size = incoming_size;

        // Exit since we can't receive it.
        PROCESS_EXIT();
    }

    // Respond with checksum to confirm valid program size.
    checksum = pbio_util_get_checksum(PBIO_UTIL_CHECKSUM_TYPE_XOR8_START_00, size_buf, sizeof(size_buf));
    PROCESS_WAIT_UNTIL(({
        rtx_size = 1;
        pbsys_bluetooth_tx(&checksum, &rtx_size) == PBIO_SUCCESS;
    }));

    // Reset stored size since data is garbage if reception fails.
    set_stored_program_size(0);

    // Receive program chunk by chunk.
    remaining_size = incoming_size;
    while (remaining_size) {

        etimer_set(&timer, 500);

        // Size of chunk to receive now, and location of that chunk.
        chunk_size = remaining_size < PBSYS_CONFIG_DOWNLOAD_CHUNK_SIZE ?
            remaining_size : PBSYS_CONFIG_DOWNLOAD_CHUNK_SIZE;
        chunk_buf = map->program_data + incoming_size - remaining_size;

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
    set_stored_program_size(incoming_size);
    last_received_size = incoming_size;

    PROCESS_END();
}

void pbsys_program_load_receive_start(void) {
    // Start the process.
    process_start(&pbsys_program_receive_process);
}

bool pbsys_program_load_receive_complete(void) {

    // Stop receiving the program on shutdown.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        process_exit(&pbsys_program_receive_process);
        last_received_size = PSYS_PROGRAM_LOAD_TYPE_NONE;
        return true;
    }

    // Check if the process is complete.
    if (!process_is_running(&pbsys_program_receive_process)) {
        return true;
    }

    // REVISIT: Explicit polling should not be needed if we
    // use the bluetooth callbacks to get events posted.
    process_poll(&pbsys_program_receive_process);
    return false;
}

// This part of the program info is always the same, so set it here.
static pbsys_program_load_info_t info = {
    .sys_stack_start = (uint8_t *)&_sstack,
    .sys_stack_end = (uint8_t *)&_estack,
    .sys_heap_end = (uint8_t *)&_heap_end,
};

pbsys_program_load_info_t *pbsys_program_load_get_info(void) {

    // Handle valid programs.
    if (last_received_size < PROGRAM_SIZE_MAX && map->program_size < PROGRAM_SIZE_MAX) {
        // Load info from data map.
        info.program_type = PSYS_PROGRAM_LOAD_TYPE_STORED;
        info.program_size = map->program_size;
        info.program_data = map->program_data;
        info.appl_heap_start = info.program_data + info.program_size;
        return &info;
    }

    // Handle special programs.
    if (last_received_size == PSYS_PROGRAM_LOAD_TYPE_BUILTIN_0) {
        info.program_type = PSYS_PROGRAM_LOAD_TYPE_BUILTIN_0;
        // Even though the special program has no data, we don't want to
        // let it override any other existing data.
        info.appl_heap_start = map->program_data + map->program_size;
        return &info;
    }

    // All other sizes are invalid.
    info.program_type = PSYS_PROGRAM_LOAD_TYPE_NONE;
    return &info;
}

#endif // PBSYS_CONFIG_PROGRAM_LOAD
