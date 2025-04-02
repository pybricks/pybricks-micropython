// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _PBDRV_STACK_H_
#define _PBDRV_STACK_H_

#include <stddef.h>

#include <pbdrv/config.h>

/**
 * Gets the stack information.
 *
 * @param [out] stack_start   The start of the stack.
 * @param [out] stack_end     The end of the stack.
 */
void pbdrv_stack_get_info(char **stack_start, char **stack_end);

#endif // _PBDRV_STACK_H_
