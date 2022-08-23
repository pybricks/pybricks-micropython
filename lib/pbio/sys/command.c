// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <assert.h>
#include <stdint.h>

#include <pbio/protocol.h>
#include "program_stop.h"

/**
 * Parses binary data for command and dispatches handler for command.
 * @param [in]  data    The raw command data.
 * @param [in]  size    The size of @p data in bytes.
 */
void pbsys_command(const uint8_t *data, uint32_t size) {
    assert(data);
    assert(size);

    pbio_pybricks_command_t cmd = data[0];

    switch (cmd) {
        case PBIO_PYBRICKS_COMMAND_STOP_USER_PROGRAM:
            pbsys_program_stop();
            break;
        default:
            // TODO: return error
            break;
    }
}
