// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Keeps track of overall system status.

#include <stdbool.h>

#include <pbsys/status.h>

static pbsys_status_flag_t pbsys_status_flags;

/**
 * Sets a system status flag.
 * @param [in]  flag    The flag to set.
 */
void pbsys_status_set_flag(pbsys_status_flag_t flag) {
    pbsys_status_flags |= flag;
}

/**
 * Clears a system status flag.
 * @param [in]  flag    The flag to clear.
 */
void pbsys_status_clear_flag(pbsys_status_flag_t flag) {
    pbsys_status_flags &= ~flag;
}

/**
 * Tests if flags are set.
 * @param [in]  flags   The flags to to test.
 * @return              *true* if all @p flags are set, otherwise *false*.
 */
bool pbsys_status_test_flags(pbsys_status_flag_t flags) {
    return (pbsys_status_flags & flags) == flags;
}
