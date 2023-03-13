// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 LEGO System A/S

#include <stddef.h>

#include <pbio/error.h>
#include <pbio/parent.h>

void pbio_parent_set(pbio_parent_t *self, void *parent_object, pbio_parent_stop_func_t stop_func) {
    self->parent_object = parent_object;
    self->parent_stop_func = stop_func;
}

bool pbio_parent_exists(const pbio_parent_t *self) {
    return self->parent_object != NULL;
}

bool pbio_parent_equals(const pbio_parent_t *self, const void *candidate_object) {
    return self->parent_object == candidate_object;
}

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
