// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_STACK_EMBEDDED

#include <stdint.h>

void pbdrv_stack_get_info(char **stack_start, char **stack_end) {
    // Defined in linker script.
    extern uint32_t pbdrv_stack_end;
    extern uint32_t pbdrv_stack_start;
    *stack_start = (char *)&pbdrv_stack_start;
    *stack_end = (char *)&pbdrv_stack_end;
}

#endif // PBDRV_CONFIG_STACK_EMBEDDED
