// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbsys/config.h>

#if PBSYS_CONFIG_STORAGE

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <contiki.h>

#include <pbdrv/block_device.h>
#include <pbio/main.h>
#include <pbio/protocol.h>
#include <pbio/version.h>
#include <pbsys/main.h>
#include <pbsys/storage.h>
#include <pbsys/status.h>

#include "core.h"

/**
 * Map of loaded data sits at the start of user RAM.
 */
union {
    pbsys_storage_data_map_t data_map;
    uint8_t data[PBSYS_CONFIG_STORAGE_RAM_SIZE];
} pbsys_user_ram_data_map __attribute__((section(".noinit"), used));

static pbsys_storage_data_map_t *map = &pbsys_user_ram_data_map.data_map;

static bool data_map_is_loaded = false;

pbsys_storage_settings_t *pbsys_storage_settings_get_settings(void) {
    if (!data_map_is_loaded) {
        return NULL;
    }
    return &map->settings;
}

/**
 * Requests that storage (program, user data, settings) will be saved (some
 * time before shutdown). Should be called by functions that change data.
 *
 * This is done by setting the write size to how much data must be written on
 * shutdown. This is not simply a boolean flag because it is also used as the
 * load size on boot.
 */
void pbsys_storage_request_write(void) {
    map->write_size = sizeof(pbsys_storage_data_map_t) + map->program_size;
}

/**
 * Saves user data. This will be saved during power off, like program data.
 *
 * @param [in]  offset  Offset from the base address.
 * @param [in]  data    The data to be stored (copied).
 * @param [in]  size    Data size.
 * @returns             ::PBIO_ERROR_INVALID_ARG if the data won't fit.
 *                      Otherwise, ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_storage_set_user_data(uint32_t offset, const uint8_t *data, uint32_t size) {
    if (offset + size > sizeof(map->user_data)) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Update data and write size to request write on poweroff.
    memcpy(map->user_data + offset, data, size);
    pbsys_storage_request_write();
    return PBIO_SUCCESS;
}

/**
 * Gets pointer to user data, settings, or program.
 *
 * @param [in]  offset  Offset from the base address.
 * @param [in]  data    The data reference.
 * @param [in]  size    Data size.
 * @returns             ::PBIO_ERROR_INVALID_ARG if reading out of range.
 *                      Otherwise, ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_storage_get_user_data(uint32_t offset, uint8_t **data, uint32_t size) {
    // User is allowed to read beyond user storage to include settings and
    // program data.
    if (offset + size > (map->program_data - map->user_data) + map->program_size) {
        return PBIO_ERROR_INVALID_ARG;
    }
    *data = map->user_data + offset;
    return PBIO_SUCCESS;
}

#if PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM
// Updates checksum in data map to satisfy bootloader requirements.
static void pbsys_storage_update_checksum(void) {

    // Align writable data by a double word, to simplify checksum
    // computation and storage drivers that write double words.
    while (map->write_size % 8) {
        *((uint8_t *)map + map->write_size++) = 0;
    }

    // The area scanned by the bootloader adds up to 0 when all user data
    // is 0xFFFFFFFF. So the bootloader value up until just before the user
    // data is always 0 + the number of words in the scanned user data.
    extern uint32_t _pbsys_storage_checked_size;
    uint32_t checksize = (uint32_t)&_pbsys_storage_checked_size;
    uint32_t checksum = checksize / sizeof(uint32_t);

    // Don't count existing value.
    map->checksum_complement = 0;

    // Add checksum for each word in the written data and empty checked size.
    for (uint32_t offset = 0; offset < checksize; offset += sizeof(uint32_t)) {
        uint32_t *word = (uint32_t *)((uint8_t *)map + offset);
        // Assume that everything after written data is erased by the block
        // device driver prior to writing.
        checksum += offset < map->write_size ? *word : 0xFFFFFFFF;
    }

    // Set the checksum complement to cancel out user data checksum.
    map->checksum_complement = 0xFFFFFFFF - checksum + 1;
}
#endif // PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM

/**
 * Writes the user program metadata.
 *
 * @param [in]  size    The size of the user program in bytes.
 *
 * @returns             ::PBIO_ERROR_BUSY if the user program is running.
 *                      Otherwise, ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_storage_set_program_size(uint32_t size) {
    // we can't allow this to be changed while a user program is running
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    // Update program size.
    map->program_size = size;

    // Program size was updated, so request write.
    pbsys_storage_request_write();

    return PBIO_SUCCESS;
}

/**
 * Writes program data to user RAM.
 *
 * Should be combined with at least one call to ::pbsys_storage_set_program_size.
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
pbio_error_t pbsys_storage_set_program_data(uint32_t offset, const void *data, uint32_t size) {
    if (offset + size > PBSYS_STORAGE_MAX_PROGRAM_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // We can't allow this to be changed while a user program is running.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    memcpy(map->program_data + offset, data, size);

    return PBIO_SUCCESS;
}

/**
 * Asserts that the loaded/stored user program is valid and ready to run.
 *
 * @returns                 ::PBIO_ERROR_INVALID_ARG if loaded program is not
 *                          valid. Otherwise ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_storage_assert_program_valid(void) {
    // Don't run invalid programs.
    if (map->program_size == 0 || map->program_size > PBSYS_STORAGE_MAX_PROGRAM_SIZE) {
        // TODO: Validate the data beyond just size.
        return PBIO_ERROR_INVALID_ARG;
    }
    return PBIO_SUCCESS;
}

/**
 * Populates the program data with references to the loaded program data.
 *
 * @param [in]  offset      The program data structure.
 */
void pbsys_storage_get_program_data(pbsys_main_program_t *program) {
    program->code_start = map->program_data;
    program->code_end = map->program_data + map->program_size;
    program->data_end = map->program_data + PBSYS_STORAGE_MAX_PROGRAM_SIZE;
}

PROCESS(pbsys_storage_process, "storage");

/**
 * Starts loading the user data from storage to RAM.
 */
void pbsys_storage_init(void) {
    pbsys_init_busy_up();
    process_start(&pbsys_storage_process);
}

/**
 * Starts saving the user data from RAM to storage.
 */
void pbsys_storage_deinit(void) {
    pbsys_init_busy_up();
    process_post(&pbsys_storage_process, PROCESS_EVENT_CONTINUE, NULL);
}

/**
 * This process loads data from storage on boot, and saves it on shutdown.
 */
PROCESS_THREAD(pbsys_storage_process, ev, data) {

    static pbio_error_t err;
    static struct pt pt;

    PROCESS_BEGIN();

    // Read size of stored data.
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, sizeof(map->write_size), &err));

    // Read the available data into RAM.
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, map->write_size, &err));
    if (err != PBIO_SUCCESS) {
        map->program_size = 0;
    }

    // Reset write size, so we don't write data if nothing changed.
    map->write_size = 0;

    // Test that storage matches current firmware version.
    if (map->stored_firmware_version != PBIO_HEXVERSION) {
        // Reset storage except for program data. It is sufficient to set its
        // size to 0, which is what happens here since it is in the map.
        memset(map, 0, sizeof(pbsys_storage_data_map_t));
        pbsys_storage_settings_set_defaults(&map->settings);

        // Set firmware version used to create current storage map.
        map->stored_firmware_version = PBIO_HEXVERSION;

        // Ensure new firmware version and default settings are written.
        pbsys_storage_request_write();
    }

    // Apply loaded settings as necesary.
    pbsys_storage_settings_apply_loaded_settings(&map->settings);

    // Poke processes that await on system settings to become available.
    data_map_is_loaded = true;
    process_post(PROCESS_BROADCAST, PROCESS_EVENT_COM, NULL);

    // Initialization done.
    pbsys_init_busy_down();

    // Wait for signal on signal.
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);

    // Write data to storage if it was updated.
    if (map->write_size) {

        #if PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM
        pbsys_storage_update_checksum();
        #endif

        // Write the data.
        PROCESS_PT_SPAWN(&pt, pbdrv_block_device_store(&pt, (uint8_t *)map, map->write_size, &err));
    }

    // Deinitialization done.
    pbsys_init_busy_down();

    PROCESS_END();
}

#endif // PBSYS_CONFIG_STORAGE
