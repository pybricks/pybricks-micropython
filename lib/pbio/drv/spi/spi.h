// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_SPI_H_
#define _INTERNAL_PBDRV_SPI_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/spi.h>
#include <pbio/error.h>

#if PBDRV_CONFIG_SPI

void pbdrv_spi_init(void);

#else // PBDRV_CONFIG_SPI

#define pbdrv_spi_init()

#endif // PBDRV_CONFIG_SPI

#endif // _INTERNAL_PBDRV_SPI_H_
