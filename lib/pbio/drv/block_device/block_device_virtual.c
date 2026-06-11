// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Block device driver for virtual hub on POSIX systems.
// Reads and writes a block_device.bin file in the current directory.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_VIRTUAL

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <linux/limits.h>

#include "../sys/storage_data.h"

#include <pbio/error.h>
#include <pbio/os.h>

static char block_device_file[PATH_MAX];

static struct {
    /**
     * How much data to write on shutdown and load on the next boot. Includes
     * the size of this field, because it is also saved.
     */
    uint32_t saved_size;
    /**
     * A copy of the data loaded from storage and application heap. The first
     * portion of this, up to pbdrv_block_device_get_writable_size() bytes,
     * gets saved to the file at shutdown.
     */
    union {
        // ensure that data is properly aligned for pbsys_storage_data_map_t
        pbsys_storage_data_map_t data_map;
        uint8_t data[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
    };
} ramdisk;

uint32_t pbdrv_block_device_get_writable_size(void) {
    return PBDRV_CONFIG_BLOCK_DEVICE_VIRTUAL_SIZE - sizeof(ramdisk.saved_size);
}

void pbdrv_block_device_init(void) {
    memset(&ramdisk, 0xFF, sizeof(ramdisk));

    // Resolve path to block_device.bin next to the executable.
    char exe[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (len > 0) {
        exe[len] = '\0';
        snprintf(block_device_file, sizeof(block_device_file), "%s/block_device.bin", dirname(exe));
    }
}

pbio_error_t pbdrv_block_device_get_data(pbsys_storage_data_map_t **data) {

    *data = &ramdisk.data_map;

    FILE *f = fopen(block_device_file, "rb");
    if (!f) {
        // No file found; ramdisk remains at default state.
        return PBIO_SUCCESS;
    }

    // Determine actual file size via seek for cross-validation.
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 0 || (uint32_t)file_size > PBDRV_CONFIG_BLOCK_DEVICE_VIRTUAL_SIZE) {
        fclose(f);
        return PBIO_ERROR_INVALID_ARG;
    }

    // Read saved_size field first to cross-validate against the actual file size.
    uint32_t saved_size;
    if (fread(&saved_size, 1, sizeof(saved_size), f) != sizeof(saved_size)) {
        fclose(f);
        return PBIO_ERROR_IO;
    }

    // saved_size must be consistent with the file size on disk.
    if (saved_size != (uint32_t)file_size) {
        fclose(f);
        return PBIO_ERROR_INVALID_ARG;
    }

    // saved_size already read and validated; set it and load the data portion.
    ramdisk.saved_size = saved_size;
    size_t read = fread(&ramdisk.data, 1, (size_t)file_size - sizeof(saved_size), f);
    fclose(f);

    if (read != (size_t)file_size - sizeof(saved_size)) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_block_device_write_all(pbio_os_state_t *state, uint32_t used_data_size) {
    // Total size includes used data and the saved_size field itself.
    uint32_t size = used_data_size + sizeof(ramdisk.saved_size);

    if (size > PBDRV_CONFIG_BLOCK_DEVICE_VIRTUAL_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Store the size so we can validate it on the next load.
    ramdisk.saved_size = size;

    FILE *f = fopen(block_device_file, "wb");
    if (!f) {
        return PBIO_ERROR_IO;
    }

    // Write saved_size and data separately to avoid any compiler padding
    // between the two fields affecting the on-disk layout.
    if (fwrite(&ramdisk.saved_size, 1, sizeof(ramdisk.saved_size), f) != sizeof(ramdisk.saved_size) ||
        fwrite(&ramdisk.data, 1, used_data_size, f) != used_data_size) {
        fclose(f);
        return PBIO_ERROR_IO;
    }

    fclose(f);
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_VIRTUAL
