// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2026 The Pybricks Authors

// BlueKitchen BTstack config for the pbio unit tests.

#ifndef _PLATFORM_TEST_BTSTACK_CONFIG_H_
#define _PLATFORM_TEST_BTSTACK_CONFIG_H_

// Native build, so BTstack allocates rather than drawing on the static pools
// that the shared config sizes.
#define HAVE_ASSERT
#define HAVE_MALLOC
#define HAVE_POSIX_FILE_IO

#define ENABLE_LOG_DEBUG
#define ENABLE_LOG_INFO
#define ENABLE_LOG_ERROR

// Nothing pairs here, and the one device in the database is the test host.
#define MAX_NR_SM_LOOKUP_ENTRIES 0
#define MAX_NR_LE_DEVICE_DB_ENTRIES 1

#include <drv/bluetooth/bluetooth_btstack_config.h>

#endif // _PLATFORM_TEST_BTSTACK_CONFIG_H_
