// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Software button implementation for simulating buttons in tests

#include <pbio/button.h>
#include <pbio/error.h>

static pbio_button_flags_t pbio_test_button_flags;

void pbio_test_button_set_pressed(pbio_button_flags_t flags) {
    pbio_test_button_flags = flags;
}

void _pbdrv_button_init(void) {
}

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    *pressed = pbio_test_button_flags;
    return PBIO_SUCCESS;
}
