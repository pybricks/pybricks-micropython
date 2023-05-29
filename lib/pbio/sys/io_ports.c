// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

// Manages I/O ports

#include <pbsys/status.h>

#include "../drv/ioport/ioport.h"

void pbsys_io_ports_poll(void) {
    if (pbsys_status_test(PBIO_PYBRICKS_STATUS_SHUTDOWN)) {
        pbdrv_ioport_deinit();
    }
}
