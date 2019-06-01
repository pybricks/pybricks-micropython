// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBIO_UARTDEV_H_
#define _PBIO_UARTDEV_H_

#include <pbio/config.h>

#if PBIO_CONFIG_UARTDEV

#include "sys/process.h"

/** @cond INTERNAL */

PROCESS_NAME(pbio_uartdev_process);

/** @endcond */

#endif // PBIO_CONFIG_UARTDEV

#endif // _PBIO_UARTDEV_H_
