// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Block device driver for the internal flash of the AT91SAM7S256 on the NXT.
//
// The NXT has no external storage, so this uses a region at the top of the
// same 256K flash plane that the firmware runs from.
//
// Flash endurance is limited (on the order of 10k cycles), but higher level
// code only writes on shutdown and only when something actually changed, so
// this sees no more wear than the original firmware did.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_NXT

#if PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE < PBDRV_CONFIG_BLOCK_DEVICE_NXT_SIZE + 2048
#error "Application RAM not big enough."
#endif

#include <stdint.h>
#include <string.h>

#include <at91sam7s256.h>
#include <nxos/interrupts.h>

#include "../rproc/rproc_nxt.h"
#include "../sys/storage_data.h"

#include <pbdrv/block_device.h>

#include <pbio/error.h>
#include <pbio/os.h>

/** Start of the flash plane. Page numbers are counted from here. */
#define FLASH_BASE_ADDRESS (0x00100000)

/** Erase and program granularity, in bytes. */
#define FLASH_PAGE_SIZE (256)

extern uint8_t _pbdrv_block_device_storage_start[];

static struct {
    /**
     * How much data to write on shutdown and load on the next boot. Includes
     * the size of this field, because it is also saved.
     */
    uint32_t saved_size;
    /**
     * A copy of the data loaded from flash and application heap. The first
     * portion of this, up to pbdrv_block_device_get_writable_size() bytes,
     * gets saved to flash at shutdown.
     */
    union {
        // ensure that data is properly aligned for pbsys_storage_data_map_t
        pbsys_storage_data_map_t data_map;
        uint8_t data[PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE];
    };
} ramdisk __attribute__((section(".noinit"), used));

uint32_t pbdrv_block_device_get_writable_size(void) {
    return PBDRV_CONFIG_BLOCK_DEVICE_NXT_SIZE - sizeof(ramdisk.saved_size);
}

static pbio_error_t init_err;

void pbdrv_block_device_init(void) {

    uint32_t size;

    memcpy(&size, _pbdrv_block_device_storage_start, sizeof(size));

    // Exit on invalid size, which includes the erased state of a hub that
    // never saved anything. This error will be retrieved when higher level
    // code requests the ramdisk, so that it can reset data to firmware
    // defaults.
    if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_NXT_SIZE) {
        init_err = PBIO_ERROR_INVALID_ARG;
        return;
    }

    // Load requested amount of data to RAM. Also re-reads the size value.
    memcpy(&ramdisk, _pbdrv_block_device_storage_start, size);
}

pbio_error_t pbdrv_block_device_get_data(pbsys_storage_data_map_t **data) {
    *data = &ramdisk.data_map;
    return init_err;
}

/**
 * Starts the write page command and waits for it to complete.
 *
 * This runs from RAM because the flash plane cannot be read while it is being
 * programmed. For the same reason the caller must disable interrupts, since
 * the interrupt handlers live in flash.
 *
 * The flash mode register is configured once during startup, with the erase
 * before programming enabled, so each page is erased as part of this command.
 *
 * @param [in]  page    Page number counted from the start of the flash plane.
 * @return              Flash status register value, with the error flags of
 *                      all reads combined since reading clears them.
 */
static uint32_t __attribute__((noinline, section(".ram_text"))) pbdrv_block_device_nxt_write_page(uint32_t page) {

    AT91C_BASE_MC->MC_FCR = (0x5A << 24) | ((page << 8) & AT91C_MC_PAGEN) | AT91C_MC_FCMD_START_PROG;

    // Programming a page takes a few milliseconds. There is no time source
    // available with interrupts disabled, so the wait is bounded by a loop
    // count well beyond that at 48 MHz.
    uint32_t status = 0;
    for (uint32_t i = 0; i < 1000000; i++) {
        uint32_t value = AT91C_BASE_MC->MC_FSR;
        status |= value;
        if (value & AT91C_MC_FRDY) {
            break;
        }
    }
    return status;
}

pbio_error_t pbdrv_block_device_write_all(pbio_os_state_t *state, uint32_t used_data_size) {

    static uint32_t done;

    uint32_t size;
    uint32_t status;
    uint32_t flags;
    volatile uint32_t *latch;
    const uint32_t *source;

    PBIO_OS_ASYNC_BEGIN(state);

    // Total size includes used data and the size field itself. A whole page is
    // always programmed, so round up instead of leaving the tail of the last
    // page undefined.
    size = used_data_size + sizeof(ramdisk.saved_size);
    ramdisk.saved_size = (size + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE * FLASH_PAGE_SIZE;

    if (ramdisk.saved_size > PBDRV_CONFIG_BLOCK_DEVICE_NXT_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    memset((uint8_t *)&ramdisk + size, 0, ramdisk.saved_size - size);

    for (done = 0; done < ramdisk.saved_size; done += FLASH_PAGE_SIZE) {

        // Interrupts are off for a few milliseconds per page, which stalls the
        // AVR link. The AVR only gives up after two seconds without a valid
        // packet, so the link survives this as long as no transfer is aborted
        // part way through: losing it takes the power supply with it. Yield so
        // the link gets a turn, then start only once it is between transfers.
        // Nothing yields in between, so it cannot start one behind our back.
        PBIO_OS_AWAIT_ONCE(state);
        PBIO_OS_AWAIT_UNTIL(state, pbdrv_rproc_nxt_link_is_idle());

        // Fill the write latch buffer. This does not touch the flash plane;
        // nothing is committed until the write page command below. The latch
        // buffer accepts 32-bit accesses only.
        latch = (volatile uint32_t *)(void *)(_pbdrv_block_device_storage_start + done);
        source = (const uint32_t *)(const void *)((uint8_t *)&ramdisk + done);
        for (uint32_t i = 0; i < FLASH_PAGE_SIZE / sizeof(uint32_t); i++) {
            latch[i] = source[i];
        }

        flags = nx_interrupts_disable();
        status = pbdrv_block_device_nxt_write_page(
            ((uint32_t)_pbdrv_block_device_storage_start - FLASH_BASE_ADDRESS + done) / FLASH_PAGE_SIZE);
        nx_interrupts_enable(flags);

        // FRDY not rising means the controller timed out. LOCKE means the lock
        // region covering the storage area is locked, in which case nothing
        // was written. PROGE means a bad command, which would be a bug here.
        if (!(status & AT91C_MC_FRDY) || (status & (AT91C_MC_LOCKE | AT91C_MC_PROGE))) {
            return PBIO_ERROR_IO;
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_NXT
