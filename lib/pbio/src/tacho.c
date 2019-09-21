// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <inttypes.h>

#include <fixmath.h>

#include <pbio/port.h>
#include <pbio/tacho.h>

static pbio_tacho_t tachos[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_tacho_setup(pbio_tacho_t *tacho, uint8_t counter_id, pbio_direction_t direction, fix16_t counts_per_degree, fix16_t gear_ratio) {
    // Assert that scaling factors are positive
    if (gear_ratio < 0 || counts_per_degree < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Get overal ratio from counts to output variable, including gear train
    tacho->counts_per_output_unit = fix16_mul(counts_per_degree, gear_ratio);

    // Configure direction
    tacho->direction = direction;

    // Get counter device
    pbio_error_t err = pbdrv_counter_get(tacho->counter_id, &tacho->counter);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set the offset such that tacho output is 0 or absolute starting point
    // TODO: call reset

    return PBIO_SUCCESS;
}

pbio_error_t pbio_tacho_get(pbio_port_t port, pbio_tacho_t **tacho, pbio_direction_t direction, fix16_t counts_per_degree, fix16_t gear_ratio) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to tacho
    *tacho = &tachos[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    
    // FIXME: Make proper way to get counter id
    uint8_t counter_id = port - PBDRV_CONFIG_FIRST_MOTOR_PORT;

    // Initialize and set up tacho properties
    return pbio_tacho_setup(*tacho, counter_id, direction, counts_per_degree, gear_ratio);
}
