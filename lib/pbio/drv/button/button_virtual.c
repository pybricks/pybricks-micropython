// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_VIRTUAL

#include <pbio/button.h>
#include <pbio/error.h>

#include "../virtual.h"

void _pbdrv_button_init(void) {
}

pbio_error_t pbdrv_button_is_pressed(pbio_button_flags_t *pressed) {
    *pressed = pbdrv_virtual_get_unsigned_long("buttons");

    return *pressed == (pbio_button_flags_t)-1 ? PBIO_ERROR_FAILED : PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_BUTTON_VIRTUAL
