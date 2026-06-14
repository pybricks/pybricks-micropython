// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2026 The Pybricks Authors

// Block device driver for W25Qxx SPI flash memory chip connected to STM32.
//
// Two transports are supported, selected at build time:
// - Classic SPI peripheral (PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SPI), used
//   on STM32F4 based hubs.
// - XSPI/OCTOSPI peripheral (PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI), used
//   on STM32H5 based hubs.
//
// The high level flash access protothreads (read, erase/write, init) are shared
// between both transports. Only the low level command plumbing differs, so it is
// grouped into a small set of transport helper protothreads with a common
// signature.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32

#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SPI == PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI
#error "Exactly one of PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SPI or _XSPI must be set."
#endif

#if PBDRV_CONFIG_BLOCK_DEVICE_RAM_SIZE < PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_SIZE + 2048
#error "Application RAM not big enough."
#endif

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

#define DEBUG 0
#if DEBUG
#include <pbio/debug.h>
#define BDEV_TRACE pbio_debug
#else
#define BDEV_TRACE(...)
#endif

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

// Select constant values based on flash device type.
#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_W25Q32
#define W25Qxx(Q32, Q256) (Q32)
#elif PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_W25Q256
#define W25Qxx(Q32, Q256) (Q256)
#endif

/**
 * Device-specific flash commands.
 */
enum {
    FLASH_CMD_GET_STATUS = 0x05,
    FLASH_CMD_WRITE_ENABLE = 0x06,
    FLASH_CMD_RESET_ENABLE = 0x66,
    FLASH_CMD_RESET = 0x99,
    FLASH_CMD_RELEASE_POWER_DOWN = 0xAB,
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

// Buffer for the received device ID, verified against device_id.
static uint8_t id_data[sizeof(device_id)];

// Buffer for the received status byte.
static uint8_t status;

#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

#define SPI_CMD_TIMEOUT_MS 100

// 3 or 4 byte addressing mode.
#if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_W25Q32
#define FLASH_ADDRESS_SIZE HAL_XSPI_ADDRESS_24_BITS
#else
#define FLASH_ADDRESS_SIZE HAL_XSPI_ADDRESS_32_BITS
#endif

/**
 * Per-command fields that vary; common XSPI fields are filled in at runtime.
 */
typedef struct {
    uint32_t instruction;
    uint32_t address;
    uint32_t address_mode;
    uint32_t address_width;
    uint32_t data_mode;
    uint32_t data_length;
    uint32_t dummy_cycles;
    bool receive;
    uint8_t *buffer;
} spi_command_t;

/**
 * The block device driver state.
 */
static struct {
    /** Platform-specific data */
    const pbdrv_block_device_w25qxx_stm32_platform_data_t *pdata;
    /** HAL XSPI data */
    XSPI_HandleTypeDef hxspi;
    /** HAL Transfer status */
    volatile spi_status_t spi_status;
    /** DMA for sending SPI commands and data */
    DMA_HandleTypeDef tx_dma;
    /** DMA for receiving SPI data */
    DMA_HandleTypeDef rx_dma;
} bdev;

void pbdrv_block_device_w25qxx_stm32_spi_handle_tx_dma_irq(void) {
    HAL_DMA_IRQHandler(&bdev.tx_dma);
}

void pbdrv_block_device_w25qxx_stm32_spi_handle_rx_dma_irq(void) {
    HAL_DMA_IRQHandler(&bdev.rx_dma);
}

void pbdrv_block_device_w25qxx_stm32_spi_irq(void) {
    HAL_XSPI_IRQHandler(&bdev.hxspi);
}

void pbdrv_block_device_w25qxx_stm32_spi_cmd_complete(void) {
    bdev.spi_status = SPI_STATUS_COMPLETE;
    pbio_os_request_poll();
}

void pbdrv_block_device_w25qxx_stm32_spi_tx_complete(void) {
    bdev.spi_status = SPI_STATUS_COMPLETE;
    pbio_os_request_poll();
}

void pbdrv_block_device_w25qxx_stm32_spi_rx_complete(void) {
    bdev.spi_status = SPI_STATUS_COMPLETE;
    pbio_os_request_poll();
}

void pbdrv_block_device_w25qxx_stm32_spi_error(void) {
    bdev.spi_status = SPI_STATUS_ERROR;
    pbio_os_request_poll();
}

/**
 * Initiates an XSPI transfer.
 */
static pbio_error_t spi_begin(const spi_command_t *cmd) {

    if (bdev.spi_status == SPI_STATUS_WAIT) {
        // Another operation is already in progress.
        return PBIO_ERROR_BUSY;
    }
    if (bdev.spi_status == SPI_STATUS_ERROR) {
        // Previous transmission went wrong.
        return PBIO_ERROR_IO;
    }
    // Set status to wait and start receiving.
    bdev.spi_status = SPI_STATUS_WAIT;

    HAL_StatusTypeDef err;

    XSPI_RegularCmdTypeDef xspi_cmd = {
        .OperationType = HAL_XSPI_OPTYPE_COMMON_CFG,
        .IOSelect = HAL_XSPI_SELECT_IO_3_0,
        .Instruction = cmd->instruction,
        .InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE,
        .InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS,
        .InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE,
        .Address = cmd->address,
        .AddressMode = cmd->address_mode,
        .AddressWidth = cmd->address_width,
        .AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE,
        .AlternateBytes = 0,
        .AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE,
        .AlternateBytesWidth = HAL_XSPI_ALT_BYTES_8_BITS,
        .AlternateBytesDTRMode = HAL_XSPI_ALT_BYTES_DTR_DISABLE,
        .DataMode = cmd->data_mode,
        .DataLength = cmd->data_length,
        .DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE,
        .DummyCycles = cmd->dummy_cycles,
        .DQSMode = HAL_XSPI_DQS_DISABLE,
        .SIOOMode = HAL_XSPI_SIOO_INST_EVERY_CMD,
    };

    if (cmd->instruction == 0) {
        xspi_cmd.InstructionMode = HAL_XSPI_INSTRUCTION_NONE;
    }

    BDEV_TRACE("block: cmd ins=0x%08x\r\n", cmd->instruction);
    BDEV_TRACE("block: cmd addr=0x%08x\r\n", cmd->address);
    BDEV_TRACE("block: cmd len=%u\r\n", cmd->data_length);

    // REVISIT: this is blocking. Should use HAL_XSPI_Command_IT().
    if (HAL_XSPI_Command(&bdev.hxspi, &xspi_cmd, HAL_MAX_DELAY) != HAL_OK) {
        bdev.spi_status = SPI_STATUS_COMPLETE;
        BDEV_TRACE("block: xspi command failed\r\n");
        return PBIO_ERROR_IO;
    }

    if (cmd->data_mode == HAL_XSPI_DATA_NONE || cmd->data_length == 0) {
        bdev.spi_status = SPI_STATUS_COMPLETE;
        return PBIO_SUCCESS;
    }

    if (cmd->receive) {
        if (cmd->data_length <= 4) {
            err = HAL_XSPI_Receive(&bdev.hxspi, cmd->buffer, SPI_CMD_TIMEOUT_MS);
            if (err == HAL_OK) {
                bdev.spi_status = SPI_STATUS_COMPLETE;
            } else {
                BDEV_TRACE("block: xspi rx failed\r\n");
            }
        } else {
            err = HAL_XSPI_Receive_DMA(&bdev.hxspi, cmd->buffer);
            if (err != HAL_OK) {
                BDEV_TRACE("block: xspi rxdma failed\r\n");
            }
        }
    } else {
        err = HAL_XSPI_Transmit_DMA(&bdev.hxspi, cmd->buffer);
        if (err != HAL_OK) {
            BDEV_TRACE("block: xspi txdma failed\r\n");
        }
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
 * Starts and awaits an XSPI transfer.
 */
static pbio_error_t spi_command_thread(pbio_os_state_t *state, const spi_command_t *cmd) {

    pbio_error_t err;
    static pbio_os_timer_t timer;

    PBIO_OS_ASYNC_BEGIN(state);

    // Start operation.
    err = spi_begin(cmd);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    pbio_os_timer_set(&timer, SPI_CMD_TIMEOUT_MS);

    // Wait until operation completes.
    PBIO_OS_AWAIT_UNTIL(state,
        bdev.spi_status != SPI_STATUS_WAIT ||
        pbio_os_timer_is_expired(&timer));

    if (pbio_os_timer_is_expired(&timer)) {
        bdev.spi_status = SPI_STATUS_COMPLETE;
        BDEV_TRACE("block: spi timeout\r\n");
        return PBIO_ERROR_TIMEDOUT;
    }

    if (bdev.spi_status == SPI_STATUS_ERROR) {
        bdev.spi_status = SPI_STATUS_COMPLETE;
        BDEV_TRACE("block: spi error\r\n");
        return PBIO_ERROR_IO;
    }

    if (cmd->receive && cmd->buffer && cmd->data_length) {
        BDEV_TRACE("block: rx bytes=%u first=0x%02x\r\n", cmd->data_length, cmd->buffer[0]);
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// Request flash device ID.
static spi_command_t cmd_id_read = {
    .instruction = FLASH_CMD_GET_ID,
    .address_mode = HAL_XSPI_ADDRESS_NONE,
    .address_width = HAL_XSPI_ADDRESS_8_BITS,
    .data_mode = HAL_XSPI_DATA_1_LINE,
    .data_length = sizeof(device_id),
    .receive = true,
    .buffer = id_data,
};

// Enable flash writing.
static spi_command_t cmd_write_enable = {
    .instruction = FLASH_CMD_WRITE_ENABLE,
    .address_mode = HAL_XSPI_ADDRESS_NONE,
    .address_width = HAL_XSPI_ADDRESS_8_BITS,
    .data_mode = HAL_XSPI_DATA_NONE,
};

// Reset command sequence to recover flash from unknown previous state.
static spi_command_t cmd_reset_enable = {
    .instruction = FLASH_CMD_RESET_ENABLE,
    .address_mode = HAL_XSPI_ADDRESS_NONE,
    .address_width = HAL_XSPI_ADDRESS_8_BITS,
    .data_mode = HAL_XSPI_DATA_NONE,
};

static spi_command_t cmd_reset = {
    .instruction = FLASH_CMD_RESET,
    .address_mode = HAL_XSPI_ADDRESS_NONE,
    .address_width = HAL_XSPI_ADDRESS_8_BITS,
    .data_mode = HAL_XSPI_DATA_NONE,
};

static spi_command_t cmd_release_power_down = {
    .instruction = FLASH_CMD_RELEASE_POWER_DOWN,
    .address_mode = HAL_XSPI_ADDRESS_NONE,
    .address_width = HAL_XSPI_ADDRESS_8_BITS,
    .data_mode = HAL_XSPI_DATA_NONE,
};

static spi_command_t cmd_status_read = {
    .instruction = FLASH_CMD_GET_STATUS,
    .address_mode = HAL_XSPI_ADDRESS_NONE,
    .address_width = HAL_XSPI_ADDRESS_8_BITS,
    .data_mode = HAL_XSPI_DATA_1_LINE,
    .data_length = sizeof(status),
    .receive = true,
    .buffer = &status,
};

// Read from address.
static spi_command_t cmd_read_data = {
    .instruction = FLASH_CMD_READ_DATA,
    .address_mode = HAL_XSPI_ADDRESS_1_LINE,
    .address_width = FLASH_ADDRESS_SIZE,
    .data_mode = HAL_XSPI_DATA_1_LINE,
    .dummy_cycles = 8,
    .receive = true,
};

// Write page at address.
static spi_command_t cmd_write_data = {
    .instruction = FLASH_CMD_WRITE_DATA,
    .address_mode = HAL_XSPI_ADDRESS_1_LINE,
    .address_width = FLASH_ADDRESS_SIZE,
    .data_mode = HAL_XSPI_DATA_1_LINE,
};

// Erase sector at address.
static spi_command_t cmd_erase_sector = {
    .instruction = FLASH_CMD_ERASE_BLOCK,
    .address_mode = HAL_XSPI_ADDRESS_1_LINE,
    .address_width = FLASH_ADDRESS_SIZE,
    .data_mode = HAL_XSPI_DATA_NONE,
};

static pbio_error_t flash_read_id(pbio_os_state_t *state) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_read));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_read_status(pbio_os_state_t *state) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_read));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_write_enable(pbio_os_state_t *state) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_write_enable));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_read(pbio_os_state_t *state, uint32_t address, uint8_t *buffer, uint32_t size) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    cmd_read_data.address = address;
    cmd_read_data.data_length = size;
    cmd_read_data.buffer = buffer;
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_read_data));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_write_page(pbio_os_state_t *state, uint32_t address, uint8_t *buffer, uint32_t size) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    cmd_write_data.address = address;
    cmd_write_data.data_length = size;
    cmd_write_data.buffer = buffer;
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_write_data));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_erase_sector(pbio_os_state_t *state, uint32_t address) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    cmd_erase_sector.address = address;
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_erase_sector));
    PBIO_OS_ASYNC_END(err);
}

#else // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

// 3 or 4 byte addressing mode.
#define FLASH_ADDRESS_SIZE (W25Qxx(3, 4))

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

// Request flash device ID.
static const spi_command_t cmd_id_tx = {
    .operation = SPI_SEND | SPI_CS_KEEP_ENABLED,
    .buffer = &(uint8_t) {FLASH_CMD_GET_ID},
    .size = 1,
};

// Receive flash device ID after sending request.
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
static const spi_command_t cmd_status_rx = {
    .operation = SPI_RECV,
    .buffer = &status,
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
// Should be followed by cmd_data_write to write the data.
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

static pbio_error_t flash_read_id(pbio_os_state_t *state) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_tx));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_id_rx));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_read_status(pbio_os_state_t *state) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_tx));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_status_rx));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_write_enable(pbio_os_state_t *state) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_write_enable));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_read(pbio_os_state_t *state, uint32_t address, uint8_t *buffer, uint32_t size) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    // Set address for this read request and send it.
    set_address_be(&cmd_request_read.buffer[1], address);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_request_read));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Receive the data.
    cmd_data_read.buffer = buffer;
    cmd_data_read.size = size;
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_read));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_write_page(pbio_os_state_t *state, uint32_t address, uint8_t *buffer, uint32_t size) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    // Set address and send the write request.
    set_address_be(&cmd_request_write.buffer[1], address);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_request_write));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Write the data.
    cmd_data_write.buffer = buffer;
    cmd_data_write.size = size;
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_data_write));
    PBIO_OS_ASYNC_END(err);
}

static pbio_error_t flash_erase_sector(pbio_os_state_t *state, uint32_t address) {
    static pbio_os_state_t sub;
    pbio_error_t err;
    PBIO_OS_ASYNC_BEGIN(state);
    // Set address and send the erase request.
    set_address_be(&cmd_request_erase.buffer[1], address);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_request_erase));
    PBIO_OS_ASYNC_END(err);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

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
        PBIO_OS_AWAIT(state, &sub, err = flash_read(&sub,
            PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_START_ADDRESS + offset + size_done,
            buffer + size_done, size_now));
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
    pbio_error_t err;

    PBIO_OS_ASYNC_BEGIN(state);

    // Enable write mode.
    PBIO_OS_AWAIT(state, &sub, err = flash_write_enable(&sub));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Erase (size == 0) or write the requested chunk.
    if (size == 0) {
        PBIO_OS_AWAIT(state, &sub, err = flash_erase_sector(&sub, address));
    } else {
        PBIO_OS_AWAIT(state, &sub, err = flash_write_page(&sub, address, buffer, size));
    }
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Wait for busy flag to clear.
    do {
        PBIO_OS_AWAIT(state, &sub, err = flash_read_status(&sub));
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
    #if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI
    static pbio_os_timer_t timer;
    #endif

    PBIO_OS_ASYNC_BEGIN(state);

    #if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI
    // Put flash in a known state before reading JEDEC ID.
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_release_power_down));
    if (err != PBIO_SUCCESS) {
        goto done;
    }
    PBIO_OS_AWAIT_MS(state, &timer, 2);
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_reset_enable));
    if (err != PBIO_SUCCESS) {
        goto done;
    }
    PBIO_OS_AWAIT(state, &sub, err = spi_command_thread(&sub, &cmd_reset));
    if (err != PBIO_SUCCESS) {
        goto done;
    }
    PBIO_OS_AWAIT_MS(state, &timer, 2);
    #endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

    // Read the flash device ID.
    PBIO_OS_AWAIT(state, &sub, err = flash_read_id(&sub));
    if (err != PBIO_SUCCESS) {
        goto done;
    }

    // Verify flash device ID
    if (memcmp(device_id, id_data, sizeof(id_data))) {
        err = PBIO_ERROR_FAILED;
        goto done;
    }

    // Read size of stored data.
    PBIO_OS_AWAIT(state, &sub, err = pbdrv_block_device_read(&sub, 0, (uint8_t *)&ramdisk.saved_size, sizeof(ramdisk.saved_size)));
    if (err != PBIO_SUCCESS) {
        goto done;
    }

    // Read the available data into RAM.
    PBIO_OS_AWAIT(state, &sub, err = pbdrv_block_device_read(&sub, 0, (uint8_t *)&ramdisk, ramdisk.saved_size));

    // Reading may fail with PBIO_ERROR_INVALID_ARG if the size is too big.
    // This happens when the size value was uninitialized or another firmware
    // was used before. We still want to proceed with the boot process. The
    // higher level code sees this error when requesting the RAM disk. On
    // failure, it can reset the user data to factory defaults, and save it
    // properly on shutdown.
done:
    pbio_busy_count_down();

    PBIO_OS_ASYNC_END(err);
}

void pbdrv_block_device_init(void) {

    #if PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI
    bdev.pdata = &pbdrv_block_device_w25qxx_stm32_platform_data;
    bdev.spi_status = SPI_STATUS_COMPLETE;
    bdev.tx_dma.Instance = bdev.pdata->tx_dma;
    bdev.tx_dma.Init.Request = bdev.pdata->tx_dma_req;
    bdev.tx_dma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    bdev.tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    bdev.tx_dma.Init.SrcInc = DMA_SINC_INCREMENTED;
    bdev.tx_dma.Init.DestInc = DMA_DINC_FIXED;
    bdev.tx_dma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    bdev.tx_dma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    bdev.tx_dma.Init.Priority = DMA_HIGH_PRIORITY;
    bdev.tx_dma.Init.SrcBurstLength = 1;
    bdev.tx_dma.Init.DestBurstLength = 1;
    bdev.tx_dma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    bdev.tx_dma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    bdev.tx_dma.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&bdev.tx_dma) != HAL_OK) {
        BDEV_TRACE("block: tx dma init failed\r\n");
    }
    HAL_NVIC_SetPriority(bdev.pdata->tx_dma_irq, 5, 0);
    HAL_NVIC_EnableIRQ(bdev.pdata->tx_dma_irq);
    __HAL_LINKDMA(&bdev.hxspi, hdmatx, bdev.tx_dma);

    bdev.rx_dma.Instance = bdev.pdata->rx_dma;
    bdev.rx_dma.Init.Request = bdev.pdata->rx_dma_req;
    bdev.rx_dma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    bdev.rx_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    bdev.rx_dma.Init.SrcInc = DMA_SINC_FIXED;
    bdev.rx_dma.Init.DestInc = DMA_DINC_INCREMENTED;
    bdev.rx_dma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    bdev.rx_dma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    bdev.rx_dma.Init.Priority = DMA_HIGH_PRIORITY;
    bdev.rx_dma.Init.SrcBurstLength = 1;
    bdev.rx_dma.Init.DestBurstLength = 1;
    bdev.rx_dma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    bdev.rx_dma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    bdev.rx_dma.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&bdev.rx_dma) != HAL_OK) {
        BDEV_TRACE("block: rx dma init failed\r\n");
    }
    HAL_NVIC_SetPriority(bdev.pdata->rx_dma_irq, 5, 0);
    HAL_NVIC_EnableIRQ(bdev.pdata->rx_dma_irq);
    __HAL_LINKDMA(&bdev.hxspi, hdmarx, bdev.rx_dma);

    bdev.hxspi.Instance = bdev.pdata->octospi;
    bdev.hxspi.Init.FifoThresholdByte = 1;
    bdev.hxspi.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
    bdev.hxspi.Init.MemoryType = HAL_XSPI_MEMTYPE_MICRON;
    bdev.hxspi.Init.MemorySize = HAL_XSPI_SIZE_256MB;
    bdev.hxspi.Init.ChipSelectHighTimeCycle = 3;
    bdev.hxspi.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
    bdev.hxspi.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
    bdev.hxspi.Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
    bdev.hxspi.Init.ClockPrescaler = bdev.pdata->clock_prescaler;
    bdev.hxspi.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
    bdev.hxspi.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE;
    bdev.hxspi.Init.ChipSelectBoundary = 0;
    bdev.hxspi.Init.DelayBlockBypass = HAL_XSPI_DELAY_BLOCK_BYPASS;
    bdev.hxspi.Init.Refresh = 0;
    if (HAL_XSPI_Init(&bdev.hxspi) != HAL_OK) {
        BDEV_TRACE("block: xspi init failed\r\n");
    }
    HAL_NVIC_SetPriority(bdev.pdata->irq, 6, 2);
    HAL_NVIC_EnableIRQ(bdev.pdata->irq);

    #else // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI
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
    #endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32_XSPI

    pbio_busy_count_up();
    pbio_os_process_start(&pbdrv_block_device_w25qxx_stm32_init_process, pbdrv_block_device_w25qxx_stm32_init_process_thread, NULL);
}

#endif // PBDRV_CONFIG_BLOCK_DEVICE_W25QXX_STM32
