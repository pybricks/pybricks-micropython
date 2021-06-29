// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Manages I/O ports

#include <pbsys/status.h>

// TODO need to make this more generic - for now assuming LPF2 everywhere
#include "../../drv/ioport/ioport_lpf2.h"

void pbsys_io_ports_poll(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        pbdrv_ioport_lpf2_shutdown();
    }
}
