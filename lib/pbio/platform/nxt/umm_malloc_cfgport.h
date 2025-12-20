// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// Configuration for umm_malloc memory allocator for NXT platform.
// See lib/umm_malloc/src/umm_malloc_cfg.h for details.

// We only use this for large allocations, like images, so we can use a larger
// block size to reduce overhead.
#define UMM_BLOCK_BODY_SIZE 128

#define UMM_INFO
#define DBGLOG_ENABLE
