// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _PBIO_DEBUG_H_
#define _PBIO_DEBUG_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

void pbio_debug_va(const char *format, va_list args);

void pbio_debug(const char *format, ...);

#endif // _PBIO_DEBUG_H_
