// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// Hack to allow umm_info() to print in MicroPython.

#include "py/mpprint.h"

#undef DBGLOG_FORCE
#define DBGLOG_FORCE(force, fmt, ...) mp_printf(&mp_plat_print, fmt,##__VA_ARGS__)

#define DBGLOG_32_BIT_PTR(p) (p)
