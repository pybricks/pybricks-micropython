// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

// SPI driver for STM32F4x using IRQ.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SPI_STM32F4_IRQ

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_spi.h>

#include <pbdrv/gpio.h>
#include <pbdrv/spi.h>

#include <pbio/error.h>
#include <pbio/util.h>

#include "./spi_stm32f4_irq.h"
#include "../../src/processes.h"

typedef enum {
    PBDRV_SPI_STATUS_WAIT,
    PBDRV_SPI_STATUS_COMPLETE,
    PBDRV_SPI_STATUS_ERROR,
} pbdrv_spi_status_t;

typedef struct {
    /** Public SPI device handle. */
    pbdrv_spi_dev_t spi_dev;
    /** Platform-specific data */
    const pbdrv_spi_stm32f4_irq_platform_data_t *pdata;
    /** HAL Transfer status */
    volatile pbdrv_spi_status_t rtx_status;
    /** Timer for rx/tx timeout. */
    struct timer timer;
    /** Task errors */
    pbio_error_t err;
    /** Poll hook to call on SPI IRQ. **/
    void (*hook)(void);
} pbdrv_spi_t;

static pbdrv_spi_t pbdrv_spi[PBDRV_CONFIG_SPI_STM32F4_IRQ_NUM_SPI];

pbio_error_t pbdrv_spi_get(uint8_t id, pbdrv_spi_dev_t **spi_dev) {
    if (id >= PBDRV_CONFIG_SPI_STM32F4_IRQ_NUM_SPI) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // If pdata is not set, then the driver has not been initialized.
    if (!pbdrv_spi[id].pdata) {
        return PBIO_ERROR_AGAIN;
    }

    *spi_dev = &pbdrv_spi[id].spi_dev;

    return PBIO_SUCCESS;
}

static void pbdrv_spi_chip_select(pbdrv_spi_dev_t *spi_dev, bool set) {
    pbdrv_spi_t *spi = PBIO_CONTAINER_OF(spi_dev, pbdrv_spi_t, spi_dev);

    // Active low, so set CS means set /CS low.
    if (set) {
        pbdrv_gpio_out_low(&spi->pdata->pin_ncs);
    } else {
        pbdrv_gpio_out_high(&spi->pdata->pin_ncs);
    }
}

void pbdrv_spi_stm32f4_irq_handle_txrx_complete(uint8_t id) {
    pbdrv_spi_t *spi = &pbdrv_spi[id];
    spi->rtx_status = PBDRV_SPI_STATUS_COMPLETE;
    if (spi->hook) {
        spi->hook();
    }
}

void pbdrv_spi_stm32f4_irq_handle_error(uint8_t id) {
    pbdrv_spi_t *spi = &pbdrv_spi[id];
    spi->rtx_status = PBDRV_SPI_STATUS_ERROR;
    if (spi->hook) {
        spi->hook();
    }
}

void pbdrv_spi_set_hook(pbdrv_spi_dev_t *spi_dev, void (*hook)(void)) {
    pbdrv_spi_t *spi = PBIO_CONTAINER_OF(spi_dev, pbdrv_spi_t, spi_dev);
    spi->hook = hook;
}

static pbio_error_t pbdrv_spi_begin(pbdrv_spi_dev_t *spi_dev, const pbdrv_spi_command_t *cmd) {
    pbdrv_spi_t *spi = PBIO_CONTAINER_OF(spi_dev, pbdrv_spi_t, spi_dev);

    if (spi->rtx_status == PBDRV_SPI_STATUS_WAIT) {
        // Another read operation is already in progress.
        return PBIO_ERROR_BUSY;
    }

    if (spi->rtx_status == PBDRV_SPI_STATUS_ERROR) {
        // Previous transmission went wrong.
        return PBIO_ERROR_IO;
    }

    // Set status to wait and start receiving.
    spi->rtx_status = PBDRV_SPI_STATUS_WAIT;

    // Start SPI operation.
    HAL_StatusTypeDef err;
    if (cmd->operation <= PBDRV_SPI_READ_AND_DISABLE) {
        err = HAL_SPI_Receive_IT(spi->pdata->hspi, cmd->buffer, cmd->size);
    } else {
        err = HAL_SPI_Transmit_IT(spi->pdata->hspi, cmd->buffer, cmd->size);
    }

    // Handle HAL errors.
    switch (err) {
        case HAL_OK:
            break;
        case HAL_ERROR:
            return PBIO_ERROR_IO;
        case HAL_BUSY:
            return PBIO_ERROR_BUSY;
        case HAL_TIMEOUT:
            return PBIO_ERROR_TIMEDOUT;
    }

    // Set receiving timeout.
    timer_set(&spi->timer, 100 + cmd->size / 100);

    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_spi_end(pbdrv_spi_dev_t *spi_dev) {
    pbdrv_spi_t *spi = PBIO_CONTAINER_OF(spi_dev, pbdrv_spi_t, spi_dev);

    if (spi->rtx_status != PBDRV_SPI_STATUS_COMPLETE) {
        // On timeout, raise error but set internal status
        // to complete so bus can be used again.
        if (timer_expired(&spi->timer)) {
            spi->rtx_status = PBDRV_SPI_STATUS_COMPLETE;
            return PBIO_ERROR_TIMEDOUT;
        }

        // Not done, and not timed out yet, so wait.
        return PBIO_ERROR_AGAIN;
    }

    // Success, so set and return status.
    spi->rtx_status = PBDRV_SPI_STATUS_COMPLETE;
    return PBIO_SUCCESS;
}


PT_THREAD(pbdrv_spi_command_thread(struct pt *pt, pbdrv_spi_dev_t *spi_dev, const pbdrv_spi_command_t *cmd, pbio_error_t *err)) {

    PT_BEGIN(pt);

    // Select peripheral.
    pbdrv_spi_chip_select(spi_dev, true);

    // Start SPI operation.
    *err = pbdrv_spi_begin(spi_dev, cmd);
    if (*err != PBIO_SUCCESS) {
        pbdrv_spi_chip_select(spi_dev, false);
        PT_EXIT(pt);
    }

    // Wait until SPI operation completes via interrupt or timeout.
    PT_YIELD_UNTIL(pt, (*err = pbdrv_spi_end(spi_dev)) != PBIO_ERROR_AGAIN);

    // Turn off peripheral if requested and on error.
    if (*err != PBIO_SUCCESS ||
        cmd->operation == PBDRV_SPI_READ_AND_DISABLE ||
        cmd->operation == PBDRV_SPI_WRITE_AND_DISABLE) {
        pbdrv_spi_chip_select(spi_dev, false);
    }

    PT_END(pt);
}


void pbdrv_spi_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_SPI_STM32F4_IRQ_NUM_SPI; i++) {
        const pbdrv_spi_stm32f4_irq_platform_data_t *pdata = &pbdrv_spi_stm32f4_irq_platform_data[i];
        pbdrv_spi_t *spi = &pbdrv_spi[i];
        SPI_HandleTypeDef *hspi = pdata->hspi;

        // SPI2 parameter configuration
        hspi->Instance = pdata->spi;
        hspi->Init.Mode = SPI_MODE_MASTER;
        hspi->Init.Direction = SPI_DIRECTION_2LINES;
        hspi->Init.DataSize = SPI_DATASIZE_8BIT;
        hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
        hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
        hspi->Init.NSS = SPI_NSS_SOFT;
        hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
        hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
        HAL_SPI_Init(hspi);

        HAL_NVIC_SetPriority(pdata->irq, 6, 2);
        HAL_NVIC_EnableIRQ(pdata->irq);

        spi->rtx_status = PBDRV_SPI_STATUS_COMPLETE;
        spi->pdata = pdata;
    }
}

#endif // PBDRV_CONFIG_SPI_STM32F4_IRQ
