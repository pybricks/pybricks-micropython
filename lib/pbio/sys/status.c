// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// Keeps track of overall system status.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/clock.h>

#include <pbio/os.h>

#include <pbsys/host.h>
#include <pbsys/status.h>

#include "hmi.h"
#include "light.h"
#include "storage.h"

static struct {
    /** Status indications as bit flags */
    uint32_t flags;
    /** Timestamp of when status last changed */
    uint32_t changed_time[NUM_PBIO_PYBRICKS_STATUS];
    /** Currently active program identifier, if it is running according to the flags. */
    pbio_pybricks_user_program_id_t program_id;
    /** Currently selected program slot */
    pbio_pybricks_user_program_id_t slot;
} pbsys_status;

/**
 * Let other processes and external hosts know that the status changed.
 */
static void pbsys_status_update_emit(void) {

    uint8_t buf[PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE];
    pbio_pybricks_event_status_report(buf, pbsys_status.flags, pbsys_status.program_id, pbsys_status.slot);
    pbsys_host_schedule_status_update(buf);

    // Other processes may be awaiting status changes, so poll.
    pbio_os_request_poll();
}

static void pbsys_status_update_flag(pbio_pybricks_status_flags_t status, bool set) {
    uint32_t new_flags = set ? pbsys_status.flags | PBIO_PYBRICKS_STATUS_FLAG(status) : pbsys_status.flags & ~PBIO_PYBRICKS_STATUS_FLAG(status);

    if (pbsys_status.flags == new_flags) {
        // If flags have not changed, there is nothing to do.
        return;
    }

    pbsys_status.flags = new_flags;
    pbsys_status.changed_time[status] = pbdrv_clock_get_ms();

    // Let everyone know about new flags.
    pbsys_status_update_emit();

    // Status light may need updating if flags have changed.
    pbsys_status_light_handle_status_change();
}

/**
 * Gets the Pybricks status report and writes it to @p buf.
 *
 * The buffer must be at least ::PBIO_PYBRICKS_EVENT_STATUS_REPORT_SIZE bytes.
 *
 * REVISIT: This can be dropped once ::pbsys_host_schedule_status_update is implemented.
 *
 * @param [in]  buf        The buffer to hold the binary data.
 * @return                 The number of bytes written to @p buf.
 */
uint32_t pbsys_status_get_status_report(uint8_t *buf) {
    return pbio_pybricks_event_status_report(buf, pbsys_status.flags, pbsys_status.program_id, pbsys_status.slot);
}

/**
 * Increments or decrements the currently active slot.
 *
 * It does not wrap around. This is safe to call even if the maximum or minimum
 * slot is already reached.
 *
 * @param [in]  increment   @c true for increment @c false for decrement
 */
void pbsys_status_increment_selected_slot(bool increment) {
    #if PBSYS_CONFIG_HMI_NUM_SLOTS
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_FILE_IO_IN_PROGRESS)) {
        return;
    }
    if (increment && pbsys_status.slot + 1 < PBSYS_CONFIG_HMI_NUM_SLOTS) {
        pbsys_status.slot++;
        pbsys_status_update_emit();
    }
    if (!increment && pbsys_status.slot > 0) {
        pbsys_status.slot--;
        pbsys_status_update_emit();
    }
    #endif
}

/**
 * Sets the currently active slot if it is valid.
 *
 * @param [in]  slot   Slot id to set.
 */
void pbsys_status_set_selected_slot(uint8_t slot) {
    #if PBSYS_CONFIG_HMI_NUM_SLOTS
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_FILE_IO_IN_PROGRESS)) {
        return;
    }

    if (slot < PBSYS_CONFIG_HMI_NUM_SLOTS) {
        pbsys_status.slot = slot;
        pbsys_status_update_emit();
    }
    #endif
}

/**
 * Gets the currently selected program slot.
 *
 * @return The currently selected program slot (zero-indexed).
 */
pbio_pybricks_user_program_id_t pbsys_status_get_selected_slot(void) {
    return pbsys_status.slot;
}

/**
 * Sets the identifier for the currently active program status information.
 *
 * Value only meaningful if ::PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING is set.
 *
 * @param [in]  program_id   The identifier to set.
 */
void pbsys_status_set_program_id(pbio_pybricks_user_program_id_t program_id) {
    if (pbsys_status.program_id == program_id) {
        return;
    }

    pbsys_status.program_id = program_id;
    pbsys_status_update_emit();
}

/**
 * Sets a system status status indication.
 * @param [in]  status   The status indication to set.
 */
void pbsys_status_set(pbio_pybricks_status_flags_t status) {
    assert(status < NUM_PBIO_PYBRICKS_STATUS);
    pbsys_status_update_flag(status, true);
}

/**
 * Clears a system status status indication.
 * @param [in]  status   The status indication to clear.
 */
void pbsys_status_clear(pbio_pybricks_status_flags_t status) {
    assert(status < NUM_PBIO_PYBRICKS_STATUS);
    pbsys_status_update_flag(status, false);
}

/**
 * Tests if status indication is set.
 * @param [in]  status  The status indication  to to test.
 * @return              *true* if @p status is set, otherwise *false*.
 */
bool pbsys_status_test(pbio_pybricks_status_flags_t status) {
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
bool pbsys_status_test_debounce(pbio_pybricks_status_flags_t status, bool state, uint32_t ms) {
    assert(status < NUM_PBIO_PYBRICKS_STATUS);
    if (pbsys_status_test(status) != state) {
        return false;
    }
    return pbdrv_clock_get_ms() - pbsys_status.changed_time[status] >= ms;
}

/**
 * Gets current status as bit flags.
 *
 * @return              The flags.
 */
uint32_t pbsys_status_get_flags(void) {
    return pbsys_status.flags;
}
