// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Block device driver for W25Qxx SPI flash memory chip connected to STM32.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include STM32_HAL_H

#include "../sys/storage_data.h"
#include "block_device_w25qxx_stm32.h"

#include <pbdrv/block_device.h>

#include <pbio/busy_count.h>
#include <pbio/error.h>
#include <pbio/int_math.h>

static pbio_os_process_t pbdrv_block_device_w25qxx_stm32_init_process;

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
    return PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SIZE - sizeof(ramdisk.saved_size);
}

pbio_error_t pbdrv_block_device_get_data(pbsys_storage_data_map_t **data) {
    *data = &ramdisk.data_map;

    // Higher level code can use the ramdisk data if initialization completed
    // successfully. Otherwise it should reset to factory default data.
    return pbdrv_block_device_w25qxx_stm32_init_process.err;
}

/**
 * SPI bus state.
 */
typedef enum {
    /** Operation started, bus is busy. */
    SPI_STATUS_WAIT,
    /** Operation complete, bus is idle. */
    SPI_STATUS_COMPLETE,
    /** Operation failed. */
    SPI_STATUS_ERROR,
} spi_status_t;

/**
 * Whether to receive (read) or send (write).
 */
typedef enum {
    /** Receive data from SPI device. */
    SPI_RECV = 0x00,
    /** Send data to SPI device. */
    SPI_SEND = 0x01,
    /** Bitflag to keep NCS low after the operation. Only used with SPI_SEND. */
    SPI_CS_KEEP_ENABLED = 0x02,
} spi_operation_t;

/**
 * SPI command to send or receive data with a buffer of a given size.
 */
typedef struct {
    /** Whether to read or write, and whether to keep CS enabled when done. */
    spi_operation_t operation;
    /** Buffer to write from or read into. */
    uint8_t *buffer;
    /** Buffer size. */
    uint16_t size;
} spi_command_t;

/**
 * The block device driver state.
 */
static struct {
    /** Platform-specific data */
    const pbdrv_block_device_w25qxx_stm32_platform_data_t *pdata;
    /** HAL SPI data */
    SPI_HandleTypeDef hspi;
    /** HAL Transfer status */
    volatile spi_status_t spi_status;
    /** DMA for sending SPI commands and data */
    DMA_HandleTypeDef tx_dma;
    /** DMA for receiving SPI data */
    DMA_HandleTypeDef rx_dma;
} bdev;

/**
 * Interrupt handler for SPI IRQ. Called from IRQ handler in platform.c.
 */
void pbdrv_block_device_w25qxx_stm32_spi_handle_tx_dma_irq(void) {
    HAL_DMA_IRQHandler(&bdev.tx_dma);
}

/**
 * Interrupt handler for SPI IRQ. Called from IRQ handler in platform.c.
 */
void pbdrv_block_device_w25qxx_stm32_spi_handle_rx_dma_irq(void) {
    HAL_DMA_IRQHandler(&bdev.rx_dma);
}

/**
 * Interrupt handler for SPI IRQ. Called from IRQ handler in platform.c.
 */
void pbdrv_block_device_w25qxx_stm32_spi_irq(void) {
    HAL_SPI_IRQHandler(&bdev.hspi);
}

/**
 * Tx transfer complete. Called from IRQ handler in platform.c.
 */
void pbdrv_block_device_w25qxx_stm32_spi_tx_complete(void) {
    bdev.spi_status = SPI_STATUS_COMPLETE;
    pbio_os_request_poll();
}

/**
 * Rx transfer complete. Called from IRQ handler in platform.c.
 */
void pbdrv_block_device_w25qxx_stm32_spi_rx_complete(void) {
    bdev.spi_status = SPI_STATUS_COMPLETE;
    pbio_os_request_poll();
}

/**
 * Transfer error. Called from IRQ handler in platform.c.
 */
void pbdrv_block_device_w25qxx_stm32_spi_error(void) {
    bdev.spi_status = SPI_STATUS_ERROR;
    pbio_os_request_poll();
}

/**
 * Sets or clears the chip select line. This is required for various operations
 * of the w25qxx
 *
 * @param [in] set         Whether to set (true) or clear (false) CS.
 */
static void spi_chip_select(bool set) {
    // Active low, so set CS means set /CS low.
    if (set) {
        pbdrv_gpio_out_low(&bdev.pdata->pin_ncs);
    } else {
        pbdrv_gpio_out_high(&bdev.pdata->pin_ncs);
    }
}

/**
 * Initiates an SPI transfer via DMA.
 *
 * @param [in] cmd         The command to start.
 * @return                 ::PBIO_SUCCESS on success.
 *                         ::PBIO_ERROR_BUSY if SPI is busy.
 *                         ::PBIO_ERROR_INVALID_ARG for invalid args to HAL call.
 *                         ::PBIO_ERROR_IO for other errors.
 */
static pbio_error_t spi_begin(const spi_command_t *cmd) {

    if (bdev.spi_status == SPI_STATUS_WAIT) {
        // Another read operation is already in progress.
        return PBIO_ERROR_BUSY;
    }
    if (bdev.spi_status == SPI_STATUS_ERROR) {
        // Previous transmission went wrong.
        return PBIO_ERROR_IO;
    }
    // Set status to wait and start receiving.
    bdev.spi_status = SPI_STATUS_WAIT;

    // Start SPI operation.
    HAL_StatusTypeDef err;
    if (cmd->operation == SPI_RECV) {
        err = HAL_SPI_Receive_DMA(&bdev.hspi, cmd->buffer, cmd->size);
    } else {
        err = HAL_SPI_Transmit_DMA(&bdev.hspi, cmd->buffer, cmd->size);
    }

    // Handle HAL errors.
    switch (err) {
        case HAL_OK:
            return PBIO_SUCCESS;
        case HAL_ERROR:
            return PBIO_ERROR_INVALID_ARG;
        case HAL_BUSY:
            return PBIO_ERROR_BUSY;
        default:
            return PBIO_ERROR_IO;
    }
}

/**
 * Starts and awaits an SPI transfer.
 */
static pbio_error_t spi_command_thread(pbio_os_state_t *state, const spi_command_t *cmd) {

    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Select peripheral.
    spi_chip_select(true);

    // Start SPI operation.
    err = spi_begin(cmd);
    if (err != PBIO_SUCCESS) {
        spi_chip_select(false);
        return err;
    }

    // Wait until SPI operation completes.
    PBIO_OS_AWAIT_UNTIL(state, bdev.spi_status == SPI_STATUS_COMPLETE);

    // Turn off peripheral if requested.
    if (!(cmd->operation & SPI_CS_KEEP_ENABLED)) {
        spi_chip_select(false);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// Select constant values based on flash device type.
#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_W25Q32
#define W25Qxx(Q32, Q256) (Q32)
#elif PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_W25Q256
#define W25Qxx(Q32, Q256) (Q256)
#endif

// 3 or 4 byte addressing mode.
#define FLASH_ADDRESS_SIZE (W25Qxx(3, 4))

static void set_address_be(uint8_t *buf, uint32_t address) {
    #if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_W25Q32
    buf[0] = address >> 16;
    buf[1] = address >> 8;
    buf[2] = address;
    #else
    buf[0] = address >> 24;
    buf[1] = address >> 16;
    buf[2] = address >> 8;
    buf[3] = address;
    #endif
}

/**
 * Device-specific flash commands.
 */
enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_GET_ID = 0x9F,
    FLASH_CMD_READ_DATA = W25Qxx(0x0B, 0x0C),
    FLASH_CMD_ERASE_BLOCK = W25Qxx(0x20, 0x21),
    FLASH_CMD_WRITE_DATA = W25Qxx(0x02, 0x12),
};

/**
 * Flash sizes.
 */
enum {
    FLASH_SIZE_ERASE = 4 * 1024, // Limited by W25QXX operation
    FLASH_SIZE_READ = UINT16_MAX, // Limited by STM32 DMA transfer size.
    FLASH_SIZE_WRITE = 256, // Limited by W25QXX operation
};

/**
 * Flash status flags.
 */
enum {
    FLASH_STATUS_BUSY = 0x01,
    FLASH_STATUS_WRITE_ENABLED = 0x02,
};

// W25Qxx manufacturer and device ID.
static const uint8_t device_id[] = {0xEF, 0x40, W25Qxx(0x16, 0x19)};

// Request flash device ID.
static const spi_command_t cmd_id_tx = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = &(uint8_t) {FLASH_CMD_GET_ID},
    .size = 1,
};

// Receive flash device ID after sending request.
static uint8_t id_data[sizeof(device_id)];
static const spi_command_t cmd_id_rx = {
    .operation = SPI_RECV,
    .buffer = id_data,
    .size = sizeof(id_data),
};

// Enable flash writing.
static const spi_command_t cmd_write_enable = {
    .operation = SPI_SEND,
    .buffer = &(uint8_t) {FLASH_CMD_WRITE_ENABLE},
    .size = 1,
};

// Request the write-status byte.
static const spi_command_t cmd_status_tx = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = &(uint8_t) {FLASH_CMD_GET_STATUS},
    .size = 1,
};

// Get the write-status byte.
static uint8_t status;
static const spi_command_t cmd_status_rx = {
    .operation = SPI_RECV,
    .buffer = (uint8_t *)&status,
    .size = sizeof(status),
};

// Request reading from address. Buffer: read command + address + dummy byte.
// Should be followed by another command that reads the data.
static uint8_t read_address[1 + FLASH_ADDRESS_SIZE + 1] = {FLASH_CMD_READ_DATA};
static const spi_command_t cmd_request_read = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = read_address,
    .size = sizeof(read_address),
};

// Request page write at address. Buffer: write command + address.
// Should be followed by cmd_data_read to read the data.
static uint8_t write_address[1 + FLASH_ADDRESS_SIZE] = {FLASH_CMD_WRITE_DATA};
static const spi_command_t cmd_request_write = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = write_address,
    .size = sizeof(write_address),
};

// Request sector erase at address. Buffer: erase command + address.
static uint8_t erase_address[1 + FLASH_ADDRESS_SIZE] = {FLASH_CMD_ERASE_BLOCK};
static const spi_command_t cmd_request_erase = {
    .operation = SPI_SEND,
    .buffer = erase_address,
    .size = sizeof(erase_address),
};

// Transmit data. Buffer and size should be set wherever this is used.
static spi_command_t cmd_data_write = {
    .operation = SPI_SEND,
};

// Read data. Buffer and size should be set wherever this is used.
static spi_command_t cmd_data_read = {
    .operation = SPI_RECV,
};

static pbio_error_t pbdrv_block_device_read(pbio_os_state_t *state, uint32_t offset, uint8_t *buffer, uint32_t size) {

    static pbio_os_state_t sub;
    static uint32_t size_done;
    static uint32_t size_now;
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || offset + size > PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Split up reads to maximum chunk size.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_READ);

        // Set address for this read request and send it.
        set_address_be(&cmd_request_read.buffer[1], PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_START_ADDRESS + offset + size_done);
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_request_read));
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // Receive the data.
        cmd_data_read.buffer = buffer + size_done;
        cmd_data_read.size = size_now;
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_read));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

/**
 * Write or erase one chunk of data from flash.
 *
 * In case of write, address must be aligned with a page.
 *
 * In case of erase (indicated by size = 0), address must be aligned with a sector.
 */
static pbio_error_t flash_erase_or_write(pbio_os_state_t *state, uint32_t address, uint8_t *buffer, uint32_t size) {

    static pbio_os_state_t sub;
    static const spi_command_t *cmd;
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Enable write mode.
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_write_enable));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Select either write or erase request.
    cmd = size == 0 ? &cmd_request_erase : &cmd_request_write;

    // Set address and send the request.
    set_address_be(&cmd->buffer[1], address);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, cmd));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Write the data, or skip in case of erase.
    if (size != 0) {
        cmd_data_write.buffer = buffer;
        cmd_data_write.size = size;
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_write));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Wait for busy flag to clear.
    do {
        // Send command to read status.
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_tx));
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // Read the status.
        PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_rx));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } while (status & (FLASH_STATUS_BUSY | FLASH_STATUS_WRITE_ENABLED));

    // The task is ready.
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_block_device_write_all(pbio_os_state_t *state, uint32_t used_data_size) {

    static pbio_os_state_t sub;
    static uint32_t offset;
    static uint32_t size_now;
    static uint32_t size_done;
    pbio_error_t err;

    // We're going to write the used portion of the ramdisk to flash. Includes
    // the size field itself, so add it to the total write size.
    uint8_t *buffer = (void *)&ramdisk;
    uint32_t size = used_data_size + sizeof(ramdisk.saved_size);

    PBIO_OS_ASYNC_BEGIN(state);

    // Exit on invalid size.
    if (size == 0 || size > PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SIZE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Store the new size so we know how much to load on next boot.
    ramdisk.saved_size = size;

    // Erase sector by sector.
    for (offset = 0; offset < size; offset += FLASH_SIZE_ERASE) {
        // Writing size 0 means erase.
        PBIO_OS_AWAIT(state, &sub, err = flash_erase_or_write(&sub,
            PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_START_ADDRESS + offset, NULL, 0));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Write page by page.
    for (size_done = 0; size_done < size; size_done += size_now) {
        size_now = pbio_int_math_min(size - size_done, FLASH_SIZE_WRITE);
        PBIO_OS_AWAIT(state, &sub, err = flash_erase_or_write(&sub,
            PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_START_ADDRESS + size_done, buffer + size_done, size_now));
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

pbio_error_t pbdrv_block_device_w25qxx_stm32_init_process_thread(pbio_os_state_t *state, void *context) {

    pbio_error_t err;
    static pbio_os_state_t sub;

    PBIO_OS_ASYNC_BEGIN(state);

    // Write the ID getter command
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_tx));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get ID command reply
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_rx));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Verify flash device ID
    if (memcmp(device_id, id_data, sizeof(id_data))) {
        return PBIO_ERROR_FAILED;
    }

    // Read size of stored data.
    PBIO_OS_AWAIT(state, &sub, err = pbdrv_block_device_read(&sub, 0, (uint8_t *)&ramdisk.saved_size, sizeof(ramdisk.saved_size)));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Read the available data into RAM.
    PBIO_OS_AWAIT(state, &sub, err = pbdrv_block_device_read(&sub, 0, (uint8_t *)&ramdisk, ramdisk.saved_size));

    // Reading may fail with PBIO_ERROR_INVALID_ARG if the size is too big.
    // This happens when the size value was uninitialized or another firmware
    // was used before. We still want to proceed with the boot process. The
    // higher level code sees this error when requesting the RAM disk. On
    // failure, it can reset the user data to factory defaults, and save it
    // properly on shutdown.
    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(err);
}

void pbdrv_block_device_init(void) {

    bdev.pdata = &pbdrv_block_device_w25qxx_stm32_platform_data;
    bdev.spi_status = SPI_STATUS_COMPLETE;

    bdev.tx_dma.Instance = bdev.pdata->tx_dma;
    bdev.tx_dma.Init.Channel = bdev.pdata->tx_dma_ch;
    bdev.tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    bdev.tx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    bdev.tx_dma.Init.MemInc = DMA_MINC_ENABLE;
    bdev.tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    bdev.tx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    bdev.tx_dma.Init.Mode = DMA_NORMAL;
    bdev.tx_dma.Init.Priority = DMA_PRIORITY_HIGH;
    bdev.tx_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    bdev.tx_dma.Init.MemBurst = DMA_MBURST_SINGLE;
    bdev.tx_dma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&bdev.tx_dma);
    HAL_NVIC_SetPriority(bdev.pdata->tx_dma_irq, 5, 0);
    HAL_NVIC_EnableIRQ(bdev.pdata->tx_dma_irq);
    __HAL_LINKDMA(&bdev.hspi, hdmatx, bdev.tx_dma);

    bdev.rx_dma.Instance = bdev.pdata->rx_dma;
    bdev.rx_dma.Init.Channel = bdev.pdata->rx_dma_ch;
    bdev.rx_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    bdev.rx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    bdev.rx_dma.Init.MemInc = DMA_MINC_ENABLE;
    bdev.rx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    bdev.rx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    bdev.rx_dma.Init.Mode = DMA_NORMAL;
    bdev.rx_dma.Init.Priority = DMA_PRIORITY_HIGH;
    bdev.rx_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    bdev.rx_dma.Init.MemBurst = DMA_MBURST_SINGLE;
    bdev.rx_dma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&bdev.rx_dma);
    HAL_NVIC_SetPriority(bdev.pdata->rx_dma_irq, 5, 0);
    HAL_NVIC_EnableIRQ(bdev.pdata->rx_dma_irq);
    __HAL_LINKDMA(&bdev.hspi, hdmarx, bdev.rx_dma);

    bdev.hspi.Instance = bdev.pdata->spi;
    bdev.hspi.Init.Mode = SPI_MODE_MASTER;
    bdev.hspi.Init.Direction = SPI_DIRECTION_2LINES;
    bdev.hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    bdev.hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    bdev.hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
    bdev.hspi.Init.NSS = SPI_NSS_SOFT;
    bdev.hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    bdev.hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    HAL_SPI_Init(&bdev.hspi);
    HAL_NVIC_SetPriority(bdev.pdata->irq, 6, 2);
    HAL_NVIC_EnableIRQ(bdev.pdata->irq);

    pbio_busy_count_up();
    pbio_os_process_start(&pbdrv_block_device_w25qxx_stm32_init_process, pbdrv_block_device_w25qxx_stm32_init_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32
