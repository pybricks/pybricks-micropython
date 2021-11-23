// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include <stddef.h>

#include <pbio/error.h>
#include <pbio/parent.h>

void pbio_parent_set(pbio_parent_t *self, void *parent_object, pbio_parent_stop_func_t stop_func) {
    self->parent_object = parent_object;
    self->parent_stop_func = stop_func;
}

void pbio_parent_clear(pbio_parent_t *self) {
    self->parent_object = NULL;
}

pbio_error_t pbio_parent_stop(pbio_parent_t *self) {
    if (!self->parent_object) {
        // Nothing to do if there is no parent.
        return PBIO_SUCCESS;
    }

    // Call the stop function and return result.
    return self->parent_stop_func(self->parent_object);
}
