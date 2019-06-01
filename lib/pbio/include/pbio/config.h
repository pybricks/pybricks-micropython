// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#ifndef _PBIO_CONFIG_H_
#define _PBIO_CONFIG_H_

// This file should be defined by applications that use the PBIO library.
#include "pbioconfig.h"

#ifndef PBIO_CONFIG_ENABLE_SYS
#define PBIO_CONFIG_ENABLE_SYS (0)
#endif

#ifndef PBIO_CONFIG_ENABLE_DEINIT
#define PBIO_CONFIG_ENABLE_DEINIT (1)
#endif

#ifndef PBIO_CONFIG_DISABLE_UARTDEV
#define PBIO_CONFIG_DISABLE_UARTDEV (0)
#endif

#endif // _PBIO_CONFIG_H_
