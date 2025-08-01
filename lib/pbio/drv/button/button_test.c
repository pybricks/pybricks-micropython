// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Software button implementation for simulating buttons in tests

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_TEST

#include <pbio/button.h>
#include <pbio/error.h>

static pbio_button_flags_t pbio_test_button_flags;

void pbio_test_button_set_pressed(pbio_button_flags_t flags) {
    pbio_test_button_flags = flags;
}

void pbdrv_button_init(void) {
}

pbio_button_flags_t pbdrv_button_get_pressed(void) {
    return pbio_test_button_flags;
}

#endif // PBDRV_CONFIG_BUTTON_TEST
