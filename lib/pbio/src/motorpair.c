// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk


#include <pbio/motor.h>
#include <pbio/motorpair.h>

uint8_t pair_idx[PBDRV_CONFIG_NUM_MOTOR_PAIRS][2] = {PBIO_PORT_NONE};
pbio_motor_pair_t pairs[PBDRV_CONFIG_NUM_MOTOR_PAIRS];

#define NOPAIR 255

uint8_t PORTS_TO_IDX(pbio_port_t port_one, pbio_port_t port_two) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_PAIRS; i++) {
        if (pair_idx[i][0] == port_one && pair_idx[i][1] == port_two) {
            return i;
        }
    }
    return NOPAIR;
}

pbio_error_t pbio_motor_make_pair(pbio_port_t port_one, pbio_port_t port_two) {

    pbio_error_t err;

    if (PORTS_TO_IDX(port_one, port_two) != NOPAIR) {
        // Already a pair, so nothing to be done
        return PBIO_SUCCESS;
    }

    if (port_one == port_two) {
        // A pair should have two distinct motors
        return PBIO_ERROR_INVALID_PORT;
    }

    // Motors should still be connected and should have encoders, which we can test by reading their angles
    int32_t dummy_angle;
    err = pbio_motor_get_angle(port_one, &dummy_angle);
    if (err != PBIO_SUCCESS) { return err; }
    err = pbio_motor_get_angle(port_two, &dummy_angle);
    if (err != PBIO_SUCCESS) { return err; }

    // If not a pair, first unpair any pair that is claiming these motors now, if any
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_PAIRS; i++) {
        // Go through all pair_idx and check if any is using motor one or two
        if (pair_idx[i][0] == port_one ||
            pair_idx[i][1] == port_one ||
            pair_idx[i][0] == port_two ||
            pair_idx[i][1] == port_two) {

            // Coast both of the motors in that pair
            err = pbio_motor_coast(pair_idx[i][0]);
            if (err != PBIO_SUCCESS) { return err; }
            err = pbio_motor_coast(pair_idx[i][1]);
            if (err != PBIO_SUCCESS) { return err; }

            // Then unpair that set
            pair_idx[i][0] = PBIO_PORT_NONE;
            pair_idx[i][1] = PBIO_PORT_NONE;
        }
    }
    // Now we can make a pair on a free index, which always exists since we just freed it up
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_PAIRS; i++) {
        if (pair_idx[i][0] == PBIO_PORT_NONE && pair_idx[i][1] == PBIO_PORT_NONE) {
            pair_idx[i][0] = port_one;
            pair_idx[i][1] = port_two;
            return PBIO_SUCCESS;
        }
    }
    // Coast both motors of the new pair
    err = pbio_motor_coast(port_one);
    if (err != PBIO_SUCCESS) { return err; }
    err = pbio_motor_coast(port_two);
    if (err != PBIO_SUCCESS) { return err; }
    return PBIO_ERROR_INVALID_ARG;
}

pbio_error_t pbio_get_motor_pair(pbio_port_t port_one, pbio_port_t port_two, pbio_motor_pair_t* pair) {
    // Make the pair if it doesn't already exist
    pbio_error_t err = pbio_motor_make_pair(port_one, port_two);
    if (err != PBIO_SUCCESS) { return err; }
    // Get the associated pair object by looking up its index
    uint8_t idx = PORTS_TO_IDX(port_one, port_two);
    if (idx == NOPAIR) {
        return PBIO_ERROR_INVALID_PORT;
    }
    else {
        pair = &pairs[idx];
        return PBIO_SUCCESS;
    }
}
