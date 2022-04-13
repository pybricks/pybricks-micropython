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
    uint32_t int_flags;
    pbio_error_t err = pbdrv_virtual_get_u32("button", -1, "pressed", &int_flags);
    *pressed = int_flags;
    return err;
}

#endif // PBDRV_CONFIG_BUTTON_VIRTUAL
