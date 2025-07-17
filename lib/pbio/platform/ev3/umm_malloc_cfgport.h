// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// Configuration for umm_malloc memory allocator for EV3 platform.
// See lib/umm_malloc/src/umm_malloc_cfg.h for details.

// We only use this for large allocations, like images and we have a lot of RAM
// on the EV3, so we can use a larger block size to reduce overhead.
#define UMM_BLOCK_BODY_SIZE 256

#define UMM_INFO
#define DBGLOG_ENABLE
