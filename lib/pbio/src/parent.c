// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 LEGO System A/S

#include <stddef.h>

#include <pbio/error.h>
#include <pbio/parent.h>

/**
 * Sets the parent of @p self and assigns a stop callback.
 *
 * Call pbio_parent_stop() with @p clear_parent set to @c true to release
 * @p parent_object when no longer needed.
 *
 * @param [in]  self                An uninitialized ::pbio_parent_t struct of the child object.
 * @param [in]  parent_object       The parent object.
 * @param [in]  stop_func           The stop function callback.
 */
void pbio_parent_set(pbio_parent_t *self, void *parent_object, pbio_parent_stop_func_t stop_func) {
    self->parent_object = parent_object;
    self->parent_stop_func = stop_func;
}

/**
 * Tests if parent object of @p self is currently assigned.
 *
 * @param [in]  self                The ::pbio_parent_t struct of the child object.
 * @returns                         @c true if the parent is assigned, otherwise @c false.
 */
bool pbio_parent_exists(const pbio_parent_t *self) {
    return self->parent_object != NULL;
}

/**
 * Tests if the parent of @p self is the same object as @p candidate_object.
 *
 * @param [in]  self                The ::pbio_parent_t struct of the child object.
 * @param [in]  candidate_object    The other object to compare.
 * @returns                         @c true if both objects are the same object, otherwise @c false.
 */
bool pbio_parent_equals(const pbio_parent_t *self, const void *candidate_object) {
    return self->parent_object == candidate_object;
}

/**
 * Calls the stop function of the parent of @p self.
 *
 * If the parent object has already been released, this function does nothing.
 *
 * @param [in]  self                The ::pbio_parent_t struct of the child object.
 * @param [in]  clear_parent        When @c true, releases the parent object.
 * @returns                         The result of the stop callback function.
 *
 * \note Even if the stop callback function fails, the parent will still be
 * released when @p clear_parent is @c true.
 */
pbio_error_t pbio_parent_stop(pbio_parent_t *self, bool clear_parent) {
    if (!pbio_parent_exists(self)) {
        // Nothing to do if there is no parent.
        return PBIO_SUCCESS;
    }

    // Call the stop function and propagate clear parent option.
    pbio_error_t err = self->parent_stop_func(self->parent_object, clear_parent);

    // Optionally clear parent. This clears the relation between objects, which
    // is usually done after the user program ends, but it can also be used to
    // cleanly handle an object going out of scope in user programs.
    if (clear_parent) {
        self->parent_object = NULL;
    }

    // Return error from stop function.
    return err;
}
