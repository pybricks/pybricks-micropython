// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/motor.h>
#include <pbio/config.h>
#include <pbio/iodev.h>

#include <ev3dev_stretch/lego_port.h>
#include <ev3dev_stretch/sysfs.h>

#define MAX_PATH_LENGTH 120

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)

inline void _pbdrv_motor_init(void) {
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void) {
}
#endif

typedef struct _motor_t {
    int n_motor;
    bool connected;
    bool coasting;
    pbio_iodev_type_id_t id;
    FILE *f_command;
    FILE *f_duty;
} motor_t;

motor_t motors[4];

static pbio_error_t ev3dev_motor_init(motor_t *mtr, pbio_port_t port) {

    pbio_error_t err;

    // Start from not connected
    mtr->connected = false;

    // Try to find tacho motor
    err = sysfs_get_number(port, "/sys/class/tacho-motor", &mtr->n_motor);
    if (err == PBIO_SUCCESS) {
        // On success, open driver name
        FILE *f_driver_name;
        err = sysfs_open_tacho_motor_attr(&f_driver_name, mtr->n_motor, "driver_name", "r");
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Read ID string
        char driver_name[MAX_PATH_LENGTH];
        err = sysfs_read_str(f_driver_name, driver_name);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Determine motor type ID
        if (!strcmp(driver_name, "lego-ev3-l-motor")) {
            mtr->id = PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR;
        } else {
            mtr->id = PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR;
        }
        // Close driver name file
        if (fclose(f_driver_name) != 0) {
            return PBIO_ERROR_IO;
        }
        // Open command file
        err = sysfs_open_tacho_motor_attr(&mtr->f_command, mtr->n_motor, "command", "w");
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Open duty file
        err = sysfs_open_tacho_motor_attr(&mtr->f_duty, mtr->n_motor, "duty_cycle_sp", "w");
        if (err != PBIO_SUCCESS) {
            return err;

        }
    }
    // If tacho-motor was not found, look for dc-motor instead
    else if (err == PBIO_ERROR_NO_DEV) {
        // Find dc-motor
        err = sysfs_get_number(port, "/sys/class/dc-motor", &mtr->n_motor);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // On success, open relevant sysfs files and set ID type
        mtr->id = PBIO_IODEV_TYPE_ID_EV3DEV_DC_MOTOR;
        // Open command
        err = sysfs_open_dc_motor_attr(&mtr->f_command, mtr->n_motor, "command", "w");
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Open duty
        err = sysfs_open_dc_motor_attr(&mtr->f_duty, mtr->n_motor, "duty_cycle_sp", "w");
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // Could not find either type of motor, so return the error
        return err;
    }
    // We have successfully connected
    mtr->connected = true;

    // Now that we have found the motor, coast it
    mtr->coasting = true;
    return sysfs_write_str(mtr->f_command, "stop");
}

static pbio_error_t ev3dev_motor_get(motor_t **motor, pbio_port_t port) {

    // Verify port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    // Pointer to motor
    motor_t *mtr = &motors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    *motor = mtr;

    // Return if connected
    if (mtr->connected) {
        return PBIO_SUCCESS;
    }
    // If not, try initializing it once
    else {
        return ev3dev_motor_init(mtr, port);
    }
}

static pbio_error_t ev3dev_motor_connect_status(motor_t *mtr, pbio_error_t err) {
    mtr->connected = err == PBIO_SUCCESS;
    return err;
}

pbio_error_t pbdrv_motor_coast(pbio_port_t port) {
    // Get the motor and initialize if needed
    pbio_error_t err;
    motor_t *mtr;
    err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return ev3dev_motor_connect_status(mtr, err);
    }
    // Do nothing if we are already coasting
    if (mtr->coasting) {
        return PBIO_SUCCESS;
    }
    // Send the stop command to trigger coast
    mtr->coasting = true;
    err = sysfs_write_str(mtr->f_command, "stop");
    return ev3dev_motor_connect_status(mtr, err);
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    // Get the motor and initialize if needed
    pbio_error_t err;
    motor_t *mtr;
    err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return ev3dev_motor_connect_status(mtr, err);
    }
    // If we are coasting, we must first set the command to run-direct
    if (mtr->coasting) {
        err = sysfs_write_str(mtr->f_command, "run-direct");
        if (err != PBIO_SUCCESS) {
            return ev3dev_motor_connect_status(mtr, err);
        }
        mtr->coasting = false;
    }
    // Set the duty cycle value
    err = sysfs_write_int(mtr->f_duty, duty_cycle / 100);
    return ev3dev_motor_connect_status(mtr, err);
}

pbio_error_t pbdrv_motor_get_id(pbio_port_t port, pbio_iodev_type_id_t *id) {
    pbio_error_t err;
    motor_t *mtr;
    err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return ev3dev_motor_connect_status(mtr, err);
    }
    *id = mtr->id;
    return ev3dev_motor_connect_status(mtr, PBIO_SUCCESS);
}

pbio_error_t pbdrv_motor_setup(pbio_port_t port, bool is_servo) {

    // Verify port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    // Set connected status to false so we reinitialize later
    motors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT].connected = false;

    // Set port mode to dc motor or servo
    if (!is_servo) {
        return ev3dev_lego_port_configure(port, PBIO_IODEV_TYPE_ID_EV3DEV_DC_MOTOR);
    }
    return ev3dev_lego_port_configure(port, PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR);
}

#endif // PBDRV_CONFIG_MOTOR
