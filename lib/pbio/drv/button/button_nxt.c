// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_BUTTON_NXT

#include <pbio/button.h>
#include <pbio/error.h>

#include "../rproc/rproc_nxt.h"

void pbdrv_button_init(void) {
}

pbio_button_flags_t pbdrv_button_get_pressed(void) {
    return pbdrv_rproc_nxt_get_button_pressed();
}

#endif // PBDRV_CONFIG_BUTTON_NXT
