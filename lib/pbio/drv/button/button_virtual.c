// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_VIRTUAL

#include <stddef.h>

#include <pbio/button.h>

#include "../rproc/rproc_virtual.h"

pbio_button_flags_t pbdrv_button_get_pressed(void) {
    return pdrv_rproc_virtual_get_button_state();
}

void pbdrv_button_init(void) {
}

#endif // PBDRV_CONFIG_BUTTON_VIRTUAL
