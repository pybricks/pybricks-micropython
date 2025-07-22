// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>

/**
 * Busy counter. Any nonzero value means that initialization or deinitialization
 * is ongoing.
 *
 * NB: This is used by pbdrv, pbio, pbsys, but never at the same time.
 */
static uint32_t pbio_busy_count;

/**
 * Increases init or deinit reference count.
 *
 * Modules that have async (de)init must call this when (de)initialization is
 * started to indicate that (de)init is still busy.
 */
void pbio_busy_count_up(void) {
    pbio_busy_count++;
}

/**
 * Decreases init or deinit reference count.
 *
 * Modules that have async (de)init must call this when (de)initialization is
 * finished to indicate that (de)init is no longer busy.
 */
void pbio_busy_count_down(void) {
    pbio_busy_count--;
}

/**
 * Tests if init or deinitialization is still busy.
 *
 * After calling pbsys_init(), the Contiki event loop should run until this
 * returns true to wait for all drivers to be (de)initialized.
 */
bool pbio_busy_count_busy(void) {
    return pbio_busy_count;
}
