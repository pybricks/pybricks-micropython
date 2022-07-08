// Copyright (c) 2018-2020,2022 The Pybricks Authors

// helper functions for managing ev3dev output ports

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

#include <ev3dev_stretch/lego_motor.h>
#include <ev3dev_stretch/lego_port.h>
#include <ev3dev_stretch/sysfs.h>

#define MAX_PATH_LENGTH 120

typedef struct {
    int n_motor;
    bool connected;
    bool coasting;
    pbio_iodev_type_id_t id;
    FILE *f_command;
    FILE *f_duty;
} ev3dev_motor_t;

static ev3dev_motor_t motors[PBDRV_CONFIG_LAST_MOTOR_PORT - PBDRV_CONFIG_FIRST_MOTOR_PORT + 1];

static pbio_error_t ev3dev_motor_init(ev3dev_motor_t *mtr, pbio_port_id_t port) {

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

static pbio_error_t ev3dev_motor_connect_status(ev3dev_motor_t *mtr, pbio_error_t err) {
    mtr->connected = err == PBIO_SUCCESS;
    return err;
}

/**
 * Gets the motor reference for the given port.
 *
 * @param [out] mtr     The motor reference.
 * @param [in]  port    The requested port.
 * @return              Error code.
 */
static pbio_error_t ev3dev_motor_get(ev3dev_motor_t **mtr, pbio_port_id_t port) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *mtr = &motors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    pbio_error_t err = PBIO_SUCCESS;

    if (!(*mtr)->connected) {
        err = ev3dev_motor_init(*mtr, port);
    }

    return ev3dev_motor_connect_status(*mtr, err);
}

/**
 * Sets up the motor port
 * @param [in]  port     The motor port
 * @param [out] is_servo Whether the expected motor type is a servo
 * @return               ::PBIO_SUCCESS if the call was successful,
 *                       ::PBIO_ERROR_INVALID_ARG if port is not a valid port
 *                       ::PBIO_ERROR_NO_DEV if port is valid but motor is not connected
 *                       ::PBIO_ERROR_EAGAIN if this should be called again later
 *                       ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t ev3dev_motor_setup(pbio_port_id_t port, bool is_servo) {
    // Verify port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Set connected status to false so we reinitialize later
    motors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT].connected = false;

    return ev3dev_lego_port_configure(port,
        is_servo ? PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR : PBIO_IODEV_TYPE_ID_EV3DEV_DC_MOTOR);
}

/**
 * Gets the motor type ID of the currently connected motor.
 *
 * @param [in]  port        The port the motor is attached to.
 * @param [out] id          The type identifier.
 * @return                  Error code.
 */
pbio_error_t ev3dev_motor_get_id(pbio_port_id_t port, pbio_iodev_type_id_t *id) {
    ev3dev_motor_t *mtr;
    pbio_error_t err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *id = mtr->id;

    return ev3dev_motor_connect_status(mtr, PBIO_SUCCESS);
}

/**
 * Runs the motor using the "run-direct" command.
 *
 * @param [in]  port        The port the motor is attached to.
 * @param [in]  duty_cycle  The requested duty cycle -100 to 100.
 * @return                  Error code.
 */
pbio_error_t ev3dev_motor_run(pbio_port_id_t port, int duty_cycle) {
    pbio_error_t err;
    ev3dev_motor_t *mtr;

    err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return err;
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
    err = sysfs_write_int(mtr->f_duty, duty_cycle);
    if (err != PBIO_SUCCESS) {
        return ev3dev_motor_connect_status(mtr, err);
    }

    return PBIO_SUCCESS;
}

/**
 * Stops the motor using the "stop" command.
 *
 * @param [in]  port        The port the motor is attached to.
 * @return                  Error code.
 */
pbio_error_t ev3dev_motor_stop(pbio_port_id_t port) {
    pbio_error_t err;
    ev3dev_motor_t *mtr;

    err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Do nothing if we are already coasting
    if (mtr->coasting) {
        return PBIO_SUCCESS;
    }

    // Send the stop command to trigger coast
    mtr->coasting = true;
    err = sysfs_write_str(mtr->f_command, "stop");
    if (err != PBIO_SUCCESS) {
        return ev3dev_motor_connect_status(mtr, err);
    }

    return PBIO_SUCCESS;
}
