// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

/**
 * @addtogroup SPIDriver Driver: SPI master driver.
 * @{
 */

#ifndef _PBDRV_SPI_H_
#define _PBDRV_SPI_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/port.h>
#include <contiki.h>

typedef struct {

} pbdrv_spi_dev_t;

#if PBDRV_CONFIG_SPI

/**
 * Whether to read or write.
 */
typedef enum {
    /**< Read data from SPI and keep chip select enabled. */
    PBDRV_SPI_READ,
    /**< Read data from SPI and then disable chip select. */
    PBDRV_SPI_READ_AND_DISABLE,
    /**< Write data to SPI and keep chip select enabled. */
    PBDRV_SPI_WRITE,
    /**< Write data to SPI and then disable chip select. */
    PBDRV_SPI_WRITE_AND_DISABLE,
} pbdrv_spi_operation_t;

typedef struct {
    pbdrv_spi_operation_t operation;
    uint8_t *buffer;
    uint32_t size;
} pbdrv_spi_command_t;

pbio_error_t pbdrv_spi_get(uint8_t id, pbdrv_spi_dev_t **spi_dev);

void pbdrv_spi_set_hook(pbdrv_spi_dev_t *spi_dev, void (*hook)(void));

PT_THREAD(pbdrv_spi_command_thread(struct pt *pt, pbdrv_spi_dev_t *spi_dev, const pbdrv_spi_command_t *cmd, pbio_error_t *err));

#else // PBDRV_CONFIG_SPI

static inline pbio_error_t pbdrv_spi_get(uint8_t id, pbdrv_spi_dev_t **spi_dev) {
    *spi_dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_SPI

#endif // _PBDRV_SPI_H_

/** @} */
