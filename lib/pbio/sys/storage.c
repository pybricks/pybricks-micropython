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
#include "hmi.h"

/**
 * Information about one code slot.
 *
 * A size of 0 means that this slot is not used. The offset indicates where
 * the program is stored in user storage.
 *
 * Code slots are *not* stored chronologically by slot id. Instead they are
 * stored consecutively as they are received, with the newest program last.
 *
 * If a slot is already in use and a new program should be loaded into it,
 * it is deleted by mem-moving any subsequent programs into its place, and
 * appending the new program to be last again. Since a user is typically only
 * iterating code in one slot, this is therefore usually the last stored
 * program. This means memmoves happen very little, only when needed.
 *
 */
typedef struct {
    uint32_t offset;
    uint32_t size;
} pbsys_storage_slot_info_t;

/**
 * Slot at which incoming program data is currently being received.
 */
static uint8_t incoming_slot = 0;

/**
 * Map of loaded data. All data types are little-endian.
 */
typedef struct {
    /**
     * How much to write on shutdown (and how much to load on boot).
     * This must always remain the first element of this structure.
     */
    uint32_t saved_data_size;
    #if PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM
    /**
     * Checksum complement to satisfy bootloader requirements. This ensures
     * that words in the scanned area still add up to precisely 0 after user
     * data was written.
     */
    volatile uint32_t checksum_complement;
    #endif
    /**
     * End-user read-write accessible data. Everything after this is also
     * user-readable but not writable.
     */
    uint8_t user_data[PBSYS_CONFIG_STORAGE_USER_DATA_SIZE];
    /**
     * First 8 symbols of the git hash of the firmware version used to create
     * this data map. If this does not match the version of the running
     * firmware, user data will be reset to 0.
     */
    char stored_firmware_hash[8];
    /**
     * System settings. Settings will be reset to defaults when the firmware
     * version changes due to an update.
     */
    pbsys_storage_settings_t settings;
    /**
     * Size and offset info for each slot.
     */
    pbsys_storage_slot_info_t slot_info[PBSYS_CONFIG_STORAGE_NUM_SLOTS];
    /**
     * Data of the application program (code + heap).
     */
    uint8_t program_data[] __attribute__((aligned(sizeof(void *))));
} pbsys_storage_data_map_t;

/**
 * Map of loaded data.
 *
 * Throughout this file, "ROM" is used to indicate
 * non-volatile storage such as internal or external flash.
 *
 *     ▲  text / other
 *     │
 *     │  bss
 *     │
 * ram │  size info and settings   ▲ saved on   ▲
 *     │                           │ poweroff   │
 *     │  user program(s)          ▼ to "rom"   │ pbsys_user_ram_data_map
 *     │                                        │
 *     │  remaining user ram: application heap  ▼
 *     │
 *     ▼  stack
 *
 * The pbsys_user_ram_data_map union ensures that pbsys_storage_data_map_t.program_data
 * has the correct size.
 */
static union {
    /** Fully saved to ROM on poweroff. */
    pbsys_storage_data_map_t data_map;
    /** This RAM component contains several consecutive user program blobs.
     *  Those programs are saved to ROM. The remaining user heap is not saved. */
    uint8_t _size_placeholder[PBSYS_CONFIG_STORAGE_RAM_SIZE];
} pbsys_user_ram_data_map __attribute__((section(".noinit"), used));

// Application RAM must enough to load ROM and still do something useful.
#if PBSYS_CONFIG_STORAGE_RAM_SIZE < PBSYS_CONFIG_STORAGE_ROM_SIZE + 2048
#error "Application RAM must be at least ROM size + 2K."
#endif

static pbsys_storage_data_map_t *map = &pbsys_user_ram_data_map.data_map;
static bool data_map_is_loaded = false;
static bool data_map_write_on_shutdown = false;

/**
 * Gets program size or the total size of the sequentially stored slots.
 *
 * @returns             Total size of user programs.
 */
static uint32_t pbsys_storage_get_used_program_data_size(void) {
    uint32_t size = 0;
    for (uint8_t slot = 0; slot < PBSYS_CONFIG_STORAGE_NUM_SLOTS; slot++) {
        size += map->slot_info[slot].size;
    }
    return size;
}

/**
 * Gets the maximum size of all programs that can be downloaded to the hub.
 *
 * @returns             Maximum program size in bytes.
 */
uint32_t pbsys_storage_get_maximum_program_size(void) {
    // FIXME: This is the total size of *all* slots. This is not a
    // good indicator of the free space for multi-slot hubs. We need to inform
    // the host dynamically about the available size of the current slot.
    return PBSYS_CONFIG_STORAGE_ROM_SIZE - sizeof(pbsys_storage_data_map_t);
}

/**
 * Gets the persistent user settings.
 *
 * @returns             The user settings or NULL if they are not yet loaded.
 */
pbsys_storage_settings_t *pbsys_storage_settings_get_settings(void) {
    if (!data_map_is_loaded) {
        return NULL;
    }
    return &map->settings;
}

/**
 * Requests that storage (program, user data, settings) will be saved some
 * time before shutdown. Should be called by functions that change data.
 */
void pbsys_storage_request_write(void) {
    data_map_write_on_shutdown = true;
}

/**
 * Erases user data, erases user program meta data and restores user settings
 * to default.
 *
 * This resets the data in RAM, which will be saved during power off just as
 * with any other storage operations.
 *
 * This is called to initialize the data map when there is no data or when the
 * data was previously written with a different firmware version. It can also
 * be called by the user to reset the data.
 */
void pbsys_storage_reset_storage(void) {
    // Reset storage except for program data. It is sufficient to set its
    // size to 0, which is what happens here since it is in the map.
    memset(map, 0, sizeof(pbsys_storage_data_map_t));

    // Apply default settings.
    pbsys_storage_settings_set_defaults(&map->settings);

    // Set firmware version used to create current storage map.
    strncpy(map->stored_firmware_hash, pbsys_main_get_application_version_hash(), sizeof(map->stored_firmware_hash));

    // Ensure new firmware version and default settings are written on poweroff.
    pbsys_storage_request_write();
}

/**
 * Sets user data. This will be saved during power off, like program data.
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
    // program data of all slots.
    if (offset + size > (map->program_data - map->user_data) + pbsys_storage_get_used_program_data_size()) {
        return PBIO_ERROR_INVALID_ARG;
    }
    *data = map->user_data + offset;
    return PBIO_SUCCESS;
}

#if PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM
// Updates checksum in data map to satisfy bootloader requirements.
// NB: saved_data_size must be set before calculating this.
static void pbsys_storage_update_checksum(void) {

    // Align writable data by a double word, to simplify checksum
    // computation and storage drivers that write double words.
    while (map->saved_data_size % 8) {
        *((uint8_t *)map + map->saved_data_size++) = 0;
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
        checksum += offset < map->saved_data_size ? *word : 0xFFFFFFFF;
    }

    // Set the checksum complement to cancel out user data checksum.
    map->checksum_complement = 0xFFFFFFFF - checksum + 1;
}
#endif // PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM

static pbio_error_t pbsys_storage_prepare_receive(void) {

    #if PBSYS_CONFIG_STORAGE_NUM_SLOTS == 1
    map->slot_info[incoming_slot].size = 0;
    map->slot_info[incoming_slot].offset = 0;
    return PBIO_SUCCESS;
    #endif // PBSYS_CONFIG_STORAGE_NUM_SLOTS == 1

    incoming_slot = pbsys_hmi_get_selected_program_slot();

    // There are three cases:
    // - The current slot is already the last used slot
    // - The current slot is empty (so we can trivially make it the last without moving anything)
    // - The current slot has used slots after it. So "delete" current slot by move the ones after it into its spot, then load new one last.

    // Current slot will be erased (and later overwritten), so discount its size.
    uint32_t used_before_erase = pbsys_storage_get_used_program_data_size();
    uint32_t used_after_erase = used_before_erase - map->slot_info[incoming_slot].size;

    // A slot is last if its starting offset equals the remaining used space after deleting it.
    bool is_last_slot = map->slot_info[incoming_slot].offset == used_after_erase;
    bool is_empty = map->slot_info[incoming_slot].size == 0;

    // No need to move anything in these cases. Incoming program will be appended.
    if (is_empty || is_last_slot) {
        map->slot_info[incoming_slot].size = 0;
        map->slot_info[incoming_slot].offset = used_after_erase;
        return PBIO_SUCCESS;
    }

    // There could be any number of programs sequentially placed after the
    // program we will now delete.
    uint32_t remaining_programs_offset_before_erase = map->slot_info[incoming_slot].offset + map->slot_info[incoming_slot].size;
    uint32_t remaining_programs_size = used_before_erase - remaining_programs_offset_before_erase;

    // They'll be shifted into the newly available space, so shift left by
    // the size of the deleted slot.
    uint32_t gap_to_shift_left = map->slot_info[incoming_slot].size;
    uint32_t destination = map->slot_info[incoming_slot].offset;
    uint32_t source = destination + gap_to_shift_left;

    // All the slots that will be moved will have to get their offsets
    // updated by the same amount.
    for (uint8_t slot = 0; slot < PBSYS_CONFIG_STORAGE_NUM_SLOTS; slot++) {
        if (map->slot_info[slot].offset >= remaining_programs_offset_before_erase) {
            map->slot_info[slot].offset -= gap_to_shift_left;
        }
    }

    // Now move those remaining programs backwards into the "freed" space.
    memmove(map->program_data + destination, map->program_data + source, remaining_programs_size);

    // The active slot is now at the end, and ready to receive programs.
    map->slot_info[incoming_slot].size = 0;
    map->slot_info[incoming_slot].offset = used_after_erase;

    return PBIO_SUCCESS;
}

/**
 * Writes the user program metadata.
 *
 * REVISIT: At the moment, the host sends size 0, then the new program, and
 * then the new size. This implicitly protects the program from being run while
 * something is downloaded. This implementation follows that assumption. This
 * should instead be protected by a system status flag.
 *
 * @param [in]  size    The size of the user program in bytes.
 *
 * @returns             ::PBIO_ERROR_BUSY if the user program is running.
 *                      ::PBIO_ERROR_INVALID_ARG if the new program is too big.
 *                      Otherwise, ::PBIO_SUCCESS.
 */
pbio_error_t pbsys_storage_set_program_size(uint32_t new_size) {
    // we can't allow this to be changed while a user program is running
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    // Pybricks Code sends size 0 to clear the state before sending the new
    // program, then sends the size on completion.
    if (new_size == 0) {
        return pbsys_storage_prepare_receive();
    }

    // Expecting that the slot has been cleared as per the above.
    if (incoming_slot >= PBSYS_CONFIG_STORAGE_NUM_SLOTS || map->slot_info[incoming_slot].size != 0) {
        return PBIO_ERROR_FAILED;
    }

    // Word align the data.
    new_size = (new_size + 3) / 4 * 4;

    // Set information for the incoming slot.
    map->slot_info[incoming_slot].size = new_size;

    // Program download complete, so request saving on poweroff.
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
    // REVISIT: This protects against writing beyond the limit, but we should
    // be informing the host about this ahead of time instead of failing during
    // runtime.
    if (map->slot_info[incoming_slot].offset + offset + size > pbsys_storage_get_maximum_program_size()) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // We can't allow this to be changed while a user program is running.
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING)) {
        return PBIO_ERROR_BUSY;
    }

    memcpy(map->program_data + map->slot_info[incoming_slot].offset + offset, data, size);

    return PBIO_SUCCESS;
}


/**
 * Populates the program data with references to the loaded program data.
 *
 * @param [in]  offset      The program data structure.
 */
void pbsys_storage_get_program_data(pbsys_main_program_t *program) {
    //
    // REVISIT: We used to provide access to user code on the REPL. This is now
    //          extended to providing access to the active slot. Do we really
    //          want that though? Maybe REPL should just really be independent.
    //
    uint8_t slot = program->id < PBSYS_CONFIG_STORAGE_NUM_SLOTS ? program->id : pbsys_hmi_get_selected_program_slot();

    // Only requested slot is available to user.
    program->code_start = map->program_data + map->slot_info[slot].offset;
    program->code_end = map->program_data + map->slot_info[slot].offset + map->slot_info[slot].size;

    // User ram starts after the last slot.
    program->user_ram_start = map->program_data + pbsys_storage_get_used_program_data_size();
    program->user_ram_end = ((void *)&pbsys_user_ram_data_map) + sizeof(pbsys_user_ram_data_map);
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
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, sizeof(map->saved_data_size), &err));

    // Read the available data into RAM.
    PROCESS_PT_SPAWN(&pt, pbdrv_block_device_read(&pt, 0, (uint8_t *)map, map->saved_data_size, &err));

    bool is_bad_version = strncmp(map->stored_firmware_hash, pbsys_main_get_application_version_hash(), sizeof(map->stored_firmware_hash));

    // Test that storage successfully loaded and matches current firmware,
    // otherwise reset storage.
    if (err != PBIO_SUCCESS || is_bad_version) {
        pbsys_storage_reset_storage();
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
    if (data_map_write_on_shutdown) {

        map->saved_data_size = sizeof(pbsys_storage_data_map_t) + pbsys_storage_get_used_program_data_size();

        #if PBSYS_CONFIG_STORAGE_OVERLAPS_BOOTLOADER_CHECKSUM
        pbsys_storage_update_checksum();
        #endif

        // Write the data.
        PROCESS_PT_SPAWN(&pt, pbdrv_block_device_store(&pt, (uint8_t *)map, map->saved_data_size, &err));
    }

    // Deinitialization done.
    pbsys_init_busy_down();

    PROCESS_END();
}

#endif // PBSYS_CONFIG_STORAGE
