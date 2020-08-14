// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Keeps track of overall system status.

#include <assert.h>
#include <stdbool.h>

#include <contiki.h>

#include <pbsys/status.h>

static struct {
    /** Status indications as bit flags */
    uint32_t flags;
    /** Timestamp of when status last changed */
    clock_time_t changed_time[NUM_PBSYS_STATUS];
} pbsys_status;

_Static_assert(NUM_PBSYS_STATUS <= sizeof(pbsys_status.flags) * 8,
    "need to increase size of pbsys_status.flags to fit all pbsys_status_t");

static void pbsys_status_update_flag(pbsys_status_t status, uint32_t new_flags) {
    if (pbsys_status.flags == new_flags) {
        // If flags have not changed, there is nothing to do.
        return;
    }

    pbsys_status.flags = new_flags;
    pbsys_status.changed_time[status] = clock_time();
}

/**
 * Sets a system status status indication.
 * @param [in]  status   The status indication to set.
 */
void pbsys_status_set(pbsys_status_t status) {
    assert(status < NUM_PBSYS_STATUS);
    pbsys_status_update_flag(status, pbsys_status.flags | (1 << status));
}

/**
 * Clears a system status status indication.
 * @param [in]  status   The status indication to clear.
 */
void pbsys_status_clear(pbsys_status_t status) {
    assert(status < NUM_PBSYS_STATUS);
    pbsys_status_update_flag(status, pbsys_status.flags & ~(1 << status));
}

/**
 * Tests if status indication is set.
 * @param [in]  status  The status indication  to to test.
 * @return              *true* if @p status is set, otherwise *false*.
 */
bool pbsys_status_test(pbsys_status_t status) {
    assert(status < NUM_PBSYS_STATUS);
    return !!(pbsys_status.flags & (1 << status));
}

/**
 * Tests if a status indication has been set or cleared for a specified time
 * without changing.
 *
 * @param [in]  status  The status indication to to test.
 * @param [in]  state   *true* to test if the status indication is set or
 *                      *false* to test if the it is cleared.
 * @param [in]  ms      The minimum duration that the status indication has been
 *                      in the current state.
 * @return              *true* if @p status has been set to @p state for at
 *                      least @p ms, otherwise *false*.
 */
bool pbsys_status_test_debounce(pbsys_status_t status, bool state, uint32_t ms) {
    assert(status < NUM_PBSYS_STATUS);
    if (pbsys_status_test(status) != state) {
        return false;
    }
    return (clock_time() - pbsys_status.changed_time[status]) >= clock_from_msec(ms);
}
