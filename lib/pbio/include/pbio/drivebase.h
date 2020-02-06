// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_DRIVEBASE_H_
#define _PBIO_DRIVEBASE_H_

#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

/**
 * Drivebase states
 */
typedef enum {
    /* Passive control statuses: No PID Control Active */
    PBIO_DRIVEBASE_STATE_PASSIVE,
    PBIO_DRIVEBASE_STATE_ERRORED,
    /* Active control statuses: PID Control Active in non-blocking manner */   
    PBIO_DRIVEBASE_STATE_TIMED,
} pbio_drivebase_state_t;

typedef struct _pbio_drivebase_t {
    pbio_servo_t *left;
    pbio_servo_t *right;
    pbio_drivebase_state_t state;
    fix16_t wheel_diameter;
    fix16_t axle_track;
    int32_t sum_offset;
    int32_t dif_offset;
    pbio_log_t log;
    pbio_control_t control_heading;
    pbio_control_t control_distance;
} pbio_drivebase_t;

pbio_error_t pbio_drivebase_get(pbio_drivebase_t **_db, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track);

// Finite point to point control

pbio_error_t pbio_drivebase_straight(pbio_drivebase_t *db, int32_t distance);

pbio_error_t pbio_drivebase_turn(pbio_drivebase_t *db, int32_t angle);

// Ininite driving

pbio_error_t pbio_drivebase_drive(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate);

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_actuation_t after_stop);

// Measuring

pbio_error_t pbio_drivebase_get_state(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate);

pbio_error_t pbio_drivebase_reset(pbio_drivebase_t *db);

// Settings

pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t turn_rate, int32_t turn_acceleration, pbio_actuation_t stop_type);

pbio_error_t pbio_drivebase_get_drive_settings(pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *turn_rate, int32_t *turn_acceleration, pbio_actuation_t *stop_type);


void _pbio_drivebase_poll(void);

#else

static inline void _pbio_drivebase_poll(void) { }

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

#endif // _PBIO_DRIVEBASE_H_
