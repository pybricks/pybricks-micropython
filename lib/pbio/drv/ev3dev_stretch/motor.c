// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/motor.h>
#include <pbio/config.h>


#define MAX_PATH_LENGTH 120

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)

// Motor file structure for each motor
typedef struct _motor_file_t {
    pbio_iodev_type_id_t id;
    bool coasting;
    char devpath[MAX_PATH_LENGTH];
    FILE *f_encoder_count;
    FILE *f_encoder_rate;
    FILE *f_duty;
} motor_file_t;

motor_file_t motor_files[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .id = PBIO_IODEV_TYPE_ID_NONE
    }
};

// Read and append motorX to device path so the end result is /path/to/motorX
pbio_error_t sysfs_append_motor_number(DIR *dir, char *portpath, char *devpath) {
    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (ent->d_name[0] != '.') {
#pragma GCC diagnostic push
#if (__GNUC__ > 7) || (__GNUC__ == 7 && __GNUC_MINOR__ >= 1)
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
            snprintf(devpath, MAX_PATH_LENGTH, "%s%s", portpath, ent->d_name);
#pragma GCC diagnostic pop
            return PBIO_SUCCESS;
        }
    }
    return PBIO_ERROR_IO;
}

// Get the device ID and device path for the given motor port, if any
pbio_error_t sysfs_get_motor(pbio_port_t port, pbio_iodev_type_id_t *id, char *devpath) {
    char portpath[MAX_PATH_LENGTH];
    DIR *dir;

    // Check if there is a Large EV3 Motor on this port
    snprintf(portpath, MAX_PATH_LENGTH, "/sys/class/lego-port/port%d/ev3-ports:out%c:lego-ev3-l-motor/tacho-motor/", 4 + PORT_TO_IDX(port), port);
    dir = opendir(portpath);
    if (dir) {
        *id = PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR;
        return sysfs_append_motor_number(dir, portpath, devpath);
    }
    // Check if there is a Medium EV3 Motor on this port
    snprintf(portpath, MAX_PATH_LENGTH, "/sys/class/lego-port/port%d/ev3-ports:out%c:lego-ev3-m-motor/tacho-motor/", 4 + PORT_TO_IDX(port), port);
    dir = opendir(portpath);
    if (dir) {
        *id = PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR;
        return sysfs_append_motor_number(dir, portpath, devpath);
    }
    // Check if there is a DC Motor on this port
    snprintf(portpath, MAX_PATH_LENGTH, "/sys/class/lego-port/port%d/ev3-ports:out%c:rcx-motor/dc-motor/", 4 + PORT_TO_IDX(port), port);
    dir = opendir(portpath);
    if (dir) {
        *id = PBIO_IODEV_TYPE_ID_EV3_DC_MOTOR;
        return sysfs_append_motor_number(dir, portpath, devpath);
    }

    // If we're here, there is no device on this port
    *id = PBIO_IODEV_TYPE_ID_NONE;
    return PBIO_ERROR_NO_DEV;
}

// Open command file, write command, close.
pbio_error_t sysfs_motor_command(pbio_port_t port, const char* command) {
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    // Open the file in the directory corresponding to the specified port
    char commandpath[MAX_PATH_LENGTH];
    snprintf(commandpath, MAX_PATH_LENGTH, "%s/command", mtr_files->devpath);
    FILE* file = fopen(commandpath, "w");
    if (file != NULL && fprintf(file, "%s", command) >= 0 && fclose(file) == 0) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

// Reset motor and close files if they are open
pbio_error_t sysfs_close_and_reset(pbio_port_t port){
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    pbio_error_t err;
    switch (mtr_files->id) {
        // Do the same for Large and Medium motors
        case PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR:
        case PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
            err = sysfs_motor_command(port, "reset");
            if (err != PBIO_SUCCESS) { return err; }
            fclose(mtr_files->f_encoder_count);
            fclose(mtr_files->f_encoder_rate);
            fclose(mtr_files->f_duty);
            mtr_files->id = PBIO_IODEV_TYPE_ID_NONE;
            mtr_files->coasting = true;
            return PBIO_SUCCESS;
        case PBIO_IODEV_TYPE_ID_EV3_DC_MOTOR:
            err = sysfs_motor_command(port, "stop");
            if (err != PBIO_SUCCESS) { return err; }
            fclose(mtr_files->f_duty);
            mtr_files->id = PBIO_IODEV_TYPE_ID_NONE;
            mtr_files->coasting = true;
            return PBIO_SUCCESS;
        default:
            return PBIO_SUCCESS;
    }
}

pbio_error_t sysfs_motor_init(pbio_port_t port){
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    pbio_error_t err;

    // Reset and close motor if already open
    err = sysfs_close_and_reset(port);
    if (err != PBIO_SUCCESS) {  return err; }

    // Get the motor ID and device path
    err = sysfs_get_motor(port, &mtr_files->id, mtr_files->devpath);
    if (err != PBIO_SUCCESS) {  return err; }

    // File path character array to the relevant files
    char filepath[MAX_PATH_LENGTH];

    // Open the duty file
    snprintf(filepath, MAX_PATH_LENGTH, "%s/duty_cycle_sp", mtr_files->devpath);
    mtr_files->f_duty = fopen(filepath, "w");
    if (mtr_files->f_duty == NULL) { return PBIO_ERROR_IO; }

    // Open additional files for encoded motors
    if (mtr_files->id != PBIO_IODEV_TYPE_ID_EV3_DC_MOTOR) {
        // Open the position file
        snprintf(filepath, MAX_PATH_LENGTH, "%s/position", mtr_files->devpath);
        mtr_files->f_encoder_count = fopen(filepath, "r");
        if (mtr_files->f_encoder_count == NULL) { return PBIO_ERROR_IO; }
        // Open the speed file
        snprintf(filepath, MAX_PATH_LENGTH, "%s/speed", mtr_files->devpath);
        mtr_files->f_encoder_rate = fopen(filepath, "r");
        if (mtr_files->f_encoder_rate == NULL) { return PBIO_ERROR_IO; }
    }

    // If we're here, all files have been openened.
    return PBIO_SUCCESS;

}

void _pbdrv_motor_init(void) {
    for(pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++) {
        sysfs_motor_init(port);
    }
}

#if PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void) {
    for(pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++) {
        sysfs_close_and_reset(port);
    }
}
#endif

pbio_error_t pbdrv_motor_coast(pbio_port_t port) {
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    if (mtr_files->id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }
    mtr_files->coasting = true;
    pbio_error_t err = sysfs_motor_command(port, "stop");
    if (err == PBIO_SUCCESS) {
        // Return immediately if successful
        return PBIO_SUCCESS;
    }
    else {
        // Otherwise, try reinitializing once:
        // > If this fails, this means there is no device: PBIO_ERROR_NO_DEV
        // > If this succeeds, there was an error but the motor is still there.
        //   This can happen when a user unplugs and then reconnects a motor.
        //   This returns PBIO_ERROR_IO so pbio can reinitialize the motor.
        return sysfs_motor_init(port) == PBIO_SUCCESS ? PBIO_ERROR_IO : PBIO_ERROR_NO_DEV;
    }
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    if (mtr_files->id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }
    // If the motor is currently in coast mode, set it back to run-direct mode
    if (mtr_files->coasting) {
        if(sysfs_motor_command(port, "run-direct") != PBIO_SUCCESS) {
            return PBIO_ERROR_IO;
        }
        mtr_files->coasting = false;
    }
    // Write the duty cycle and return on success
    if (0 == fseek(mtr_files->f_duty, 0, SEEK_SET) &&
        0 <= fprintf(mtr_files->f_duty, "%d", duty_cycle/100) &&
        0 == fflush(mtr_files->f_duty)){
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

pbio_error_t pbdrv_motor_get_encoder_count(pbio_port_t port, int32_t *count) {
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    if (mtr_files->id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }
    if (mtr_files->id == PBIO_IODEV_TYPE_ID_EV3_DC_MOTOR) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }
    if (0 == fseek(mtr_files->f_encoder_count, 0, SEEK_SET) &&
        0 <= fscanf(mtr_files->f_encoder_count, "%d", count) &&
        0 == fflush(mtr_files->f_encoder_count)) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

pbio_error_t pbdrv_motor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    if (mtr_files->id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }
    if (mtr_files->id == PBIO_IODEV_TYPE_ID_EV3_DC_MOTOR) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }
    if (0 == fseek(mtr_files->f_encoder_rate, 0, SEEK_SET) &&
        0 <= fscanf(mtr_files->f_encoder_rate, "%d", rate) &&
        0 == fflush(mtr_files->f_encoder_rate)) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

pbio_error_t pbdrv_motor_get_id(pbio_port_t port, pbio_iodev_type_id_t *id) {
    motor_file_t *mtr_files = &motor_files[PORT_TO_IDX(port)];
    *id = mtr_files->id;
    if (*id == PBIO_IODEV_TYPE_ID_NONE) {
        return PBIO_ERROR_NO_DEV;
    }
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_MOTOR
