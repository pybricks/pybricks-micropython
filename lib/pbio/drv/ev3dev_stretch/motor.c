/*
 * Copyright (c) 2018 Laurens Valk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/motor.h>


#define MAX_PATH_LENGTH 50

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)

// Motor file structure for each motor
typedef struct _motor_file_t {
    bool coasting;
    int dir_number;
    bool files_open;
    FILE *f_encoder_count;
    FILE *f_encoder_rate;
    FILE *f_duty;
} motor_file_t;

motor_file_t motor_files[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .files_open = false
    }
};

// Open command file, write command, close.
pbio_error_t sysfs_motor_command(pbio_port_t port, const char* command) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    // Open the file in the directory corresponding to the specified port
    char filepath[MAX_PATH_LENGTH];
    snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/command", motor_files[PORT_TO_IDX(port)].dir_number);
    FILE* file = fopen(filepath, "w");
    if (file != NULL && fprintf(file, "%s", command) >= 0) {
        fclose(file);
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

// Close files if they are open
void close_files(pbio_port_t port){
    int port_index = PORT_TO_IDX(port);
    if (motor_files[port_index].files_open) {
        fclose(motor_files[port_index].f_encoder_count);
        fclose(motor_files[port_index].f_encoder_rate);
        fclose(motor_files[port_index].f_duty);
        motor_files[port_index].files_open = false;
    }
}



pbio_error_t sysfs_motor_init(pbio_port_t port){
    // Close files in case they are currently open
    close_files(port);
    // Open tacho-motor directory
    DIR *dp;
    struct dirent *ep;
    dp = opendir("/sys/class/tacho-motor");
    if (!dp) {
        return PBIO_ERROR_NO_DEV;
    }
    // Loop through the motorXs
    while ((ep = readdir(dp))) {
        // Ignore the . and .. folders
        if (strlen(ep->d_name) > 5) {
            // Obtain the numeric device ID from the path
            int dir_number, port_index;
            sscanf(ep->d_name, "%*5c%d",&dir_number);
            // Open the address file in this folder
            char ppath[MAX_PATH_LENGTH];
            snprintf(ppath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/address", dir_number);
            FILE* addr = fopen(ppath, "r");
            // Extract the port (14th) character, convert to numeric, and close address file
            fseek(addr, 13, SEEK_SET);
            port_index = fgetc(addr) - 'A';
            fclose(addr);

            if (port_index == PORT_TO_IDX(port)) {
                // Motor detected. Configure accordingly
                motor_files[port_index].dir_number = dir_number;
                //Debug message. Should replace with debug print
                printf("Initialized motor%d on port %c.\n", dir_number, port_index+'A');
                // Reset motor
                sysfs_motor_command(port, "reset");
                motor_files[PORT_TO_IDX(port)].coasting = true;
                // File path character array to the relevant speed, position files, etc.
                char filepath[MAX_PATH_LENGTH];
                // Open the position file
                snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/position", dir_number);
                motor_files[port_index].f_encoder_count = fopen(filepath, "r");
                // Open the speed file
                snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/speed", dir_number);
                motor_files[port_index].f_encoder_rate = fopen(filepath, "r");
                // Open the duty file
                snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/duty_cycle_sp", dir_number);
                motor_files[port_index].f_duty = fopen(filepath, "w");
                // Close tacho-motor directory when done
                closedir (dp);
                // Files are opened
                motor_files[port_index].files_open = true;
                // Return success
                return PBIO_SUCCESS;
            }
        }
    }
    // Motor not found
    closedir (dp);
    return PBIO_ERROR_NO_DEV;
}

pbio_error_t sysfs_motor_deinit(pbio_port_t port){
    // Reinitialize to trigger a motor reset, then close device files.
    if(sysfs_motor_init(port) == PBIO_SUCCESS){
        close_files(port);
    }
    return PBIO_ERROR_IO;
}

void _pbdrv_motor_init(void) {
    for(pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++) {
        sysfs_motor_init(port);
    }
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_motor_deinit(void) {
    for(pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++) {
        sysfs_motor_deinit(port);
    }
}
#endif

pbio_error_t pbdrv_motor_coast(pbio_port_t port) {
    motor_files[PORT_TO_IDX(port)].coasting = true;
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
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    // If the motor is currently in coast mode, set it back to run-direct mode
    if (motor_files[PORT_TO_IDX(port)].coasting) {
        if(sysfs_motor_command(port, "run-direct") != PBIO_SUCCESS) {
            return PBIO_ERROR_IO;
        }
        motor_files[PORT_TO_IDX(port)].coasting = false;
    }
    // Write the duty cycle and return on success
    if (0 == fseek(motor_files[PORT_TO_IDX(port)].f_duty, 0, SEEK_SET) &&
        0 <= fprintf(motor_files[PORT_TO_IDX(port)].f_duty, "%d", duty_cycle/100) &&
        0 == fflush(motor_files[PORT_TO_IDX(port)].f_duty)){
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

pbio_error_t pbdrv_motor_get_encoder_count(pbio_port_t port, int32_t *count) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (0 == fseek(motor_files[PORT_TO_IDX(port)].f_encoder_count, 0, SEEK_SET) &&
        0 <= fscanf(motor_files[PORT_TO_IDX(port)].f_encoder_count, "%d", count) && 
        0 == fflush(motor_files[PORT_TO_IDX(port)].f_encoder_count)) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

pbio_error_t pbdrv_motor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (0 == fseek(motor_files[PORT_TO_IDX(port)].f_encoder_rate, 0, SEEK_SET) &&
        0 <= fscanf(motor_files[PORT_TO_IDX(port)].f_encoder_rate, "%d", rate) &&
        0 == fflush(motor_files[PORT_TO_IDX(port)].f_encoder_rate)) {
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;    
}
