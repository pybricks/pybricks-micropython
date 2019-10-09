// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbio/button.h>
#include <pbio/config.h>
#include <pbio/error.h>

void _pbdrv_button_init(void) {
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) {
}
#endif

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    *pressed = 0;

    return PBIO_SUCCESS;
}
