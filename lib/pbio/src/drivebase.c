// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk


#include <pbio/servo.h>
#include <pbio/drivebase.h>

static pbio_motor_pair_t pair_idx[PBDRV_CONFIG_NUM_MOTOR_PAIRS];
static pbio_motor_pair_t pairs[PBDRV_CONFIG_NUM_MOTOR_PAIRS];

#define NOPAIR 255

#define PORT(m) ((m) ? (m)->port : PBIO_PORT_NONE)

static uint8_t ports_to_index(pbio_port_t port_one, pbio_port_t port_two) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_PAIRS; i++) {
        if (PORT(pair_idx[i].servo1) == port_one && PORT(pair_idx[i].servo2) == port_two) {
            return i;
        }
    }
    return NOPAIR;
}

static pbio_error_t pbio_motor_make_pair(pbio_servo_t *srv1, pbio_servo_t *srv2) {

    pbio_error_t err;

    if (ports_to_index(srv1->port, srv2->port) != NOPAIR) {
        // Already a pair, so nothing to be done
        return PBIO_SUCCESS;
    }

    if (srv1->port == srv2->port) {
        // A pair should have two distinct motors
        return PBIO_ERROR_INVALID_PORT;
    }

    // Motors should still be connected and should have encoders, which we can test by reading their angles
    int32_t dummy_angle;
    err = pbio_tacho_get_angle(srv1->tacho, &dummy_angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_tacho_get_angle(srv2->tacho, &dummy_angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If not a pair, first unpair any pair that is claiming these motors now, if any
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_PAIRS; i++) {
        // Go through all pair_idx and check if any is using motor one or two
        if (PORT(pair_idx[i].servo1) == srv1->port ||
            PORT(pair_idx[i].servo2) == srv1->port ||
            PORT(pair_idx[i].servo1) == srv2->port ||
            PORT(pair_idx[i].servo2) == srv2->port) {

            // Coast both of the motors in that pair
            err = pbio_hbridge_coast(pair_idx[i].servo1->hbridge);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_hbridge_coast(pair_idx[i].servo2->hbridge);
            if (err != PBIO_SUCCESS) {
                return err;
            }

            // Then unpair that set
            pair_idx[i].servo1 = NULL;
            pair_idx[i].servo2 = NULL;
        }
    }
    // Now we can make a pair on a free index, which always exists since we just freed it up
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_PAIRS; i++) {
        if (PORT(pair_idx[i].servo1) == PBIO_PORT_NONE && PORT(pair_idx[i].servo2) == PBIO_PORT_NONE) {
            pair_idx[i].servo1 = srv1;
            pair_idx[i].servo2 = srv2;
            return PBIO_SUCCESS;
        }
    }
    // Coast both motors of the new pair
    err = pbio_hbridge_coast(srv1->hbridge);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_hbridge_coast(srv2->hbridge);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_ERROR_INVALID_ARG;
}

pbio_error_t pbio_get_motor_pair(pbio_servo_t *srv1, pbio_servo_t *srv2, pbio_motor_pair_t* pair) {
    // Make the pair if it doesn't already exist
    pbio_error_t err = pbio_motor_make_pair(srv1, srv2);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Get the associated pair object by looking up its index
    uint8_t idx = ports_to_index(srv1->port, srv2->port);
    if (idx == NOPAIR) {
        return PBIO_ERROR_INVALID_PORT;
    }
    else {
        pair = &pairs[idx];
        return PBIO_SUCCESS;
    }
}
