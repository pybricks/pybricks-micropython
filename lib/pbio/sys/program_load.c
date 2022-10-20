// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_PROGRAM_LOAD

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <contiki.h>

#include <pbdrv/block_device.h>
#include <pbio/main.h>
#include <pbio/protocol.h>
#include <pbsys/main.h>
#include <pbsys/program_load.h>
#include <pbsys/status.h>

#include "core.h"

/**
 * Map of loaded data.
 */
typedef struct {
    /**
     * User data header.
     */
    pbsys_program_load_data_header_t header;
    /**
     * Data of the application program (code + heap).
     */
    uint8_t program_data[PBSYS_CONFIG_PROGRAM_LOAD_RAM_SIZE - sizeof(pbsys_program_load_data_header_t)] __attribute__((aligned(sizeof(void *))));
} data_map_t;

// The data map sits at the start of user RAM.
data_map_t pbsys_user_ram_data_map __attribute__((section(".noinit"), used));
static data_map_t *map = &pbsys_user_ram_data_map;

static bool pbsys_program_load_start_user_program_requested;
static bool pbsys_program_load_start_repl_requested;

#if PBSYS_CONFIG_PROGRAM_LOAD_OVERLAPS_BOOTLOADER_CHECKSUM
// Updates checksum in data map to satisfy bootloader requirements.
static void pbsys_program_load_update_checksum(void) {

    // Align writable data by a double word, to simplify checksum
    // computation and storage drivers that write double words.
    while (map->header.write_size % 8) {
        *((uint8_t *)map + map->header.write_size++) = 0;
    }

    // The area scanned by the bootloader adds up to 0 when all user data
    // is 0xFFFFFFFF. So the bootloader value up until just before the user
    // data is always 0 + the number of words in the scanned user data.
    extern uint32_t _pbsys_program_load_checked_size;
    uint32_t checksize = (uint32_t)&_pbsys_program_load_checked_size;
    uint32_t checksum = checksize / sizeof(uint32_t);

    // Don't count existing value.
    map->header.checksum_complement = 0;

    // Add checksum for each word in the written data and empty checked size.
    for (uint32_t offset = 0; offset < checksize; offset += sizeof(uint32_t)) {
        uint32_t *word = (uint32_t *)((uint8_t *)map + offset);
        // Assume that everything after written data is erased.
        checksum += offset < map->header.write_size ? *word : 0xFFFFFFFF;
    }

    // Set the checksum complement to cancel out user data checksum.
    map->header.checksum_complement = 0xFFFFFFFF - checksum + 1;
}
#endif // PBSYS_CONFIG_PROGRAM_LOAD_OVERLAPS_BOOTLOADER_CHECKSUM

/**
 * Writes the user program metadata.
 *
 * @param [in]  size    The size of the user program in bytes.
 *
 * @returns             ::PBIO_ERROR_BUSY if the user program is running.
 *                      Otherwise, ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_program_load_set_program_size(uint32_t size) {
    // we can't allow this to be changed while a user program is running
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    map->header.program_size = size;
    // Data was updated, so set the write size.
    map->header.write_size = size + sizeof(pbsys_program_load_data_header_t);

    return PBIO_SUCCESS;
}

/**
 * Writes data to user RAM.
 *
 * @param [in]  offset      The offset in bytes from the base user RAM address.
 * @param [in]  data        The data to write.
 * @param [in]  size        The size of @p data.
 *
 * @returns                 ::PBIO_ERROR_INVALID_ARG if requested @p offset and
 *                          @p size are outside of the allocated user RAM.
 *                          ::PBIO_ERROR_BUSY if the user program is running.
 *                          Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_program_load_set_program_data(uint32_t offset, const void *data, uint32_t size) {
    if (offset + size > sizeof(map->program_data)) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // we can't allow this to be changed while a user program is running
    // REVISIT: we could allocate a section of user RAM that can be changed
    // while the program is running as a way of passing data to the user program
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    memcpy(map->program_data + offset, data, size);

    return PBIO_SUCCESS;
}

/**
 * Requests to start the user program.
 *
 * @returns     ::PBIO_ERROR_BUSY if a user program is already running.
 *              ::PBIO_ERROR_INVALID_ARG if the user program has an invalid size.
 *              ::PBIO_ERROR_NOT_SUPPORTED if the program load module is disabled.
 *              Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_program_load_start_user_program(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    // Don't run invalid programs.
    if (map->header.program_size == 0 || map->header.program_size > PBSYS_PROGRAM_LOAD_MAX_PROGRAM_SIZE) {
        // TODO: Validate the data beyond just size.
        return PBIO_ERROR_INVALID_ARG;
    }

    pbsys_program_load_start_user_program_requested = true;

    return PBIO_SUCCESS;
}

/**
 * Requests to start the REPL.
 *
 * @returns     ::PBIO_ERROR_BUSY if a user program is already running.
 *              ::PBIO_ERROR_NOT_SUPPORTED if the program load module is disabled.
 *              Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_program_load_start_repl(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    pbsys_program_load_start_repl_requested = true;

    return PBIO_SUCCESS;
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
 * This process loads data from storage on boot, and saves it on shutdown.
 */
PROCESS_THREAD(pbsys_program_load_process, ev, data) {

    static pbio_error_t err;
    static struct pt pt;

    PROCESS_BEGIN();

    // Read size of stored data.
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, sizeof(map->header.write_size), &err));

    // Read the available data into RAM.
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, map->header.write_size, &err));
    if (err != PBIO_SUCCESS) {
        map->header.program_size = 0;
    }

    // Reset write size, so we don't write data if nothing changed.
    map->header.write_size = 0;

    // Initialization done.
    pbsys_init_busy_down();

    // Wait for signal on signal.
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);

    // Write data to storage if it was updated.
    if (map->header.write_size) {

        #if PBSYS_CONFIG_PROGRAM_LOAD_OVERLAPS_BOOTLOADER_CHECKSUM
        pbsys_program_load_update_checksum();
        #endif

        // Write the data.
        PROCESS_PT_SPAWN(&pt, pbdrv_block_device_store(&pt, (uint8_t *)map, map->header.write_size, &err));
    }

    // Deinitialization done.
    pbsys_init_busy_down();

    PROCESS_END();
}

/**
 * Waits for a command to start a user program or REPL.
 *
 * NOTE: this function runs the contiki event loop, so it should not be called
 * from inside an contiki process.
 *
 * @param [out] program         Program info structure to be populated.
 * @return                      ::PBIO_SUCCESS on success.
 *                              ::PBIO_ERROR_CANCELED when canceled due to shutdown request.
 *                              ::PBIO_ERROR_NOT_SUPPORTED if the program load module is disabled.
 */
pbio_error_t pbsys_program_load_wait_command(pbsys_main_program_t *program) {
    for (;;) {
        // REVISIT: this can be long waiting, so we could do a more efficient
        // wait (i.e. __WFI() on embedded system)
        pbio_do_one_event();

        if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST)) {
            return PBIO_ERROR_CANCELED;
        }

        if (pbsys_program_load_start_user_program_requested) {
            pbsys_program_load_start_user_program_requested = false;
            program->run_builtin = false;
            break;
        }

        if (pbsys_program_load_start_repl_requested) {
            pbsys_program_load_start_repl_requested = false;
            program->run_builtin = true;
            break;
        }
    }

    // REPL can also use user program (e.g. in MicroPython, import user modules)
    program->code_start = map->program_data;
    program->code_end = map->program_data + map->header.program_size;
    program->data_end = map->program_data + sizeof(map->program_data);

    return PBIO_SUCCESS;
}

#endif // PBSYS_CONFIG_PROGRAM_LOAD
