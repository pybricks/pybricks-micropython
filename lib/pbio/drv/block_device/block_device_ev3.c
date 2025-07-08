// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Block device driver for N25Q128 SPI flash memory chip connected to TI AM1808.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_EV3

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../core.h"

#include <pbdrv/block_device.h>
#include <pbdrv/gpio.h>

#include <tiam1808/edma.h>
#include <tiam1808/spi.h>
#include <tiam1808/psc.h>
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/armv5/am1808/edma_event.h>
#include <tiam1808/armv5/am1808/interrupt.h>

#include "block_device_ev3.h"
#include "../drv/gpio/gpio_ev3.h"

#include <pbio/error.h>
#include <pbio/int_math.h>

/**
 * SPI bus state.
 */
typedef enum {
    /** Operation complete, bus is idle. */
    SPI_STATUS_COMPLETE = 0,
    /** Operation failed. */
    SPI_STATUS_ERROR = 1,
} spi_status_t;

/**
 * The block device driver state.
 */
static struct {
    /** HAL Transfer status */
    volatile spi_status_t spi_status;
} bdev;


/**
 * Tx transfer complete.
 */
void pbdrv_block_device_ev3_spi_tx_complete(void) {
    // TODO
}

/**
 * Rx transfer complete.
 */
void pbdrv_block_device_ev3_spi_rx_complete(void) {
    // TODO
}

/**
 * Transfer error. // TODO: hook this up
 */
void pbdrv_block_device_ev3_spi_error(void) {
    bdev.spi_status = SPI_STATUS_ERROR;
    pbio_os_request_poll();
}


// ADC / Flash SPI0 data MOSI
static const pbdrv_gpio_t pin_spi0_mosi = PBDRV_GPIO_EV3_PIN(3, 15, 12, 8, 5);
// ADC / Flash SPI0 data MISO
static const pbdrv_gpio_t pin_spi0_miso = PBDRV_GPIO_EV3_PIN(3, 11, 8, 8, 6);
// ADC / Flash SPI0 Clock
static const pbdrv_gpio_t pin_spi0_clk = PBDRV_GPIO_EV3_PIN(3, 3, 0, 1, 8);

// ADC SPI0 chip select (active low).
static const pbdrv_gpio_t pin_spi0_ncs3 = PBDRV_GPIO_EV3_PIN(3, 27, 24, 8, 2);
// Flash SPI0 chip select (active low).
static const pbdrv_gpio_t pin_spi0_ncs0 = PBDRV_GPIO_EV3_PIN(4, 7, 4, 1, 6);

// Flash write protect (active low).
static const pbdrv_gpio_t pin_flash_nwp = PBDRV_GPIO_EV3_PIN(12, 23, 20, 5, 2);
// Flash reset/hold (active low).
static const pbdrv_gpio_t pin_flash_nhold = PBDRV_GPIO_EV3_PIN(6, 31, 28, 2, 0);


static void set_address_be(uint8_t *buf, uint32_t address) {
    buf[0] = address >> 16;
    buf[1] = address >> 8;
    buf[2] = address;
}

/**
 * Device-specific flash commands.
 */
enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_GET_ID = 0x9F,
    FLASH_CMD_READ_DATA = 0x03,
    FLASH_CMD_ERASE_BLOCK = 0xD8,
    FLASH_CMD_WRITE_DATA = 0x02,
};

/**
 * Flash sizes.
 */
enum {
    FLASH_SIZE_ERASE = 64 * 1024,
    FLASH_SIZE_READ = UINT16_MAX, // Limited by DMA transfer size.
    FLASH_SIZE_WRITE = 256,
};

/**
 * Flash status flags.
 */
enum {
    FLASH_STATUS_BUSY = 0x01,
};

// N25Q128 manufacturer and device ID.
static const uint8_t device_id[] = {0x20, 0xba, 0x18};

pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size) {

    static uint32_t size_done;
    static uint32_t size_now;
    pbio_error_t err;

    (void)err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || offset + size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Split up reads to maximum chunk size.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_READ);

        // TODO: Actually implement reading
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_block_device_store(pbio_os_state_t *state, uint8_t *buffer, uint32_t size) {

    static uint32_t offset;
    static uint32_t size_now;
    static uint32_t size_done;
    pbio_error_t err;

    (void)err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_EV3_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Erase sector by sector.
    for (offset = 0; offset < size; offset += FLASH_SIZE_ERASE) {
        // TODO: Actually implement erasing
    }

    // Write page by page.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_WRITE);

        // TODO: Actually implement writing
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static pbio_os_process_t pbdrv_block_device_ev3_init_process;

pbio_error_t pbdrv_block_device_ev3_init_process_thread(pbio_os_state_t *state, void *context) {
    pbio_error_t err;

    (void)err;
    (void)device_id;

    PBIO_OS_ASYNC_BEGIN(state);

    // TODO: Get and verify flash ID

    // Verify flash device ID // REVISIT: Fix up id_data so we can memcmp
    // if (memcmp(device_id, id_data, sizeof(id_data))) {
    //     return PBIO_ERROR_FAILED;
    // }

    // Initialization done.
    pbdrv_init_busy_down();

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_block_device_init(void) {

    // REVISIT: Init SPI and DMA with TI AM1808 API
    //
    // See display_ev3.c for inspiration and adapt settings as needed.
    //

    (void)set_address_be;
    (void)pin_flash_nhold;
    (void)pin_flash_nwp;
    (void)pin_spi0_ncs0;
    (void)pin_spi0_ncs3;
    (void)pin_spi0_clk;
    (void)pin_spi0_miso;
    (void)pin_spi0_mosi;

    bdev.spi_status = SPI_STATUS_COMPLETE;

    pbdrv_init_busy_up();
    pbio_os_process_start(&pbdrv_block_device_ev3_init_process, pbdrv_block_device_ev3_init_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_EV3
