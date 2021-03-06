// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// Keeps track of overall system status.

#include <assert.h>
#include <stdbool.h>

#include <contiki.h>

#include <pbio/event.h>
#include <pbsys/status.h>

static struct {
    /** Status indications as bit flags */
    uint32_t flags;
    /** Timestamp of when status last changed */
    clock_time_t changed_time[NUM_PBIO_PYBRICKS_STATUS];
} pbsys_status;

static void pbsys_status_update_flag(pbio_pybricks_status_t status, bool set) {
    uint32_t new_flags = set ? pbsys_status.flags | PBIO_PYBRICKS_STATUS_FLAG(status) : pbsys_status.flags & ~PBIO_PYBRICKS_STATUS_FLAG(status);

    if (pbsys_status.flags == new_flags) {
        // If flags have not changed, there is nothing to do.
        return;
    }

    pbsys_status.flags = new_flags;
    pbsys_status.changed_time[status] = clock_time();
    // REVISIT: this can drop events if event queue is full
    process_post(PROCESS_BROADCAST, set ? PBIO_EVENT_STATUS_SET : PBIO_EVENT_STATUS_CLEARED,
        (process_data_t)status);
}

/**
 * Sets a system status status indication.
 * @param [in]  status   The status indication to set.
 */
void pbsys_status_set(pbio_pybricks_status_t status) {
    assert(status < NUM_PBIO_PYBRICKS_STATUS);
    pbsys_status_update_flag(status, true);
}

/**
 * Clears a system status status indication.
 * @param [in]  status   The status indication to clear.
 */
void pbsys_status_clear(pbio_pybricks_status_t status) {
    assert(status < NUM_PBIO_PYBRICKS_STATUS);
    pbsys_status_update_flag(status, false);
}

/**
 * Tests if status indication is set.
 * @param [in]  status  The status indication  to to test.
 * @return              *true* if @p status is set, otherwise *false*.
 */
bool pbsys_status_test(pbio_pybricks_status_t status) {
    assert(status < NUM_PBIO_PYBRICKS_STATUS);
    return !!(pbsys_status.flags & PBIO_PYBRICKS_STATUS_FLAG(status));
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
bool pbsys_status_test_debounce(pbio_pybricks_status_t status, bool state, uint32_t ms) {
    assert(status < NUM_PBIO_PYBRICKS_STATUS);
    if (pbsys_status_test(status) != state) {
        return false;
    }
    return (clock_time() - pbsys_status.changed_time[status]) >= clock_from_msec(ms);
}

/**
 * Gets current status as bit flags.
 *
 * @return              The flags.
 */
uint32_t pbsys_status_get_flags(void) {
    return pbsys_status.flags;
}
