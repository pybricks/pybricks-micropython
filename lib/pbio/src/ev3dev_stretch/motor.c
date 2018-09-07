
#include <stdbool.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <pbio/motor.h>


#define MAX_PATH_LENGTH 50


// Motor file structure for each motor
typedef struct _motor_file_t {
    bool connected;
    bool coasting;
    int dir_number;
    FILE *f_encoder_count;
    FILE *f_encoder_rate;
    FILE *f_duty;
} motor_file_t;

motor_file_t motor_files[] = {
    [motorindex(PBIO_PORT_A) ... motorindex(PBIO_PORT_D)]{
        .connected=false
    }
};

pbio_motor_settings_t motor_settings[] = {
    [motorindex(PBIO_PORT_A) ... motorindex(PBIO_PORT_D)]{
        .direction = PBIO_MOTOR_DIR_NORMAL,
        .max_duty = PBIO_MAX_DUTY_HARD
    }
};

// Open file, write contents, and close it
void slow_write(pbio_port_t port, const char* filename, const char* content) {
    // Open the file in the directory corresponding to the specified port
    char filepath[MAX_PATH_LENGTH];
    snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/%s", motor_files[motorindex(port)].dir_number, filename);
    FILE* file = fopen(filepath, "w"); 
    // Write the contents to the file
    fprintf(file, content);  
    // Close the file
    fclose(file);            
}

void pbio_motor_init(void) {
    // Open tacho-motor directory
    DIR *dp;
    struct dirent *ep;
    dp = opendir ("/sys/class/tacho-motor");
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
            // Store that this motor is present and save its location
            motor_files[port_index].connected = true;
            motor_files[port_index].dir_number = dir_number;
        }
    }
    // Close tacho-motor directory when done
    closedir (dp);

    // Now that we know which motors are present, open the relevant files for reading and writing
    for(pbio_port_t port = PBIO_PORT_A; port < PBIO_PORT_D; port++) {
        int port_index = motorindex(port);
        if (motor_files[port_index].connected) {
            //Debug message. Should replace with debug print
            printf("Detected motor%d on port %c.\n", motor_files[port_index].dir_number, port_index+65);
            // Reset the motor and let stop default to coast
            slow_write(port, "command", "reset");
            slow_write(port, "stop_action", "coast");
            pbio_motor_coast(port);
            // File path character array to the relevant speed, position files, etc.
            char filepath[MAX_PATH_LENGTH];
            // Open the position file
            snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/position", motor_files[port_index].dir_number);
            motor_files[port_index].f_encoder_count = fopen(filepath, "r");
            // Open the speed file
            snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/speed", motor_files[port_index].dir_number);
            motor_files[port_index].f_encoder_rate = fopen(filepath, "r");            
            // Open the duty file
            snprintf(filepath, MAX_PATH_LENGTH, "/sys/class/tacho-motor/motor%d/duty_cycle_sp", motor_files[port_index].dir_number);
            motor_files[port_index].f_duty = fopen(filepath, "w");
        }
    }    
}

void pbio_motor_deinit(void) {
    // Close the relevant files
    for(pbio_port_t port = PBIO_PORT_A; port < PBIO_PORT_D; port++) {
        int port_index = motorindex(port);
        if (motor_files[port_index].connected) {
            // Only close files for motors that are attached
            fclose(motor_files[port_index].f_encoder_count);
            fclose(motor_files[port_index].f_encoder_rate);
            fclose(motor_files[port_index].f_duty);
            // Reset the motor
            slow_write(port, "command", "reset");
        }
    }
}

pbio_error_t pbio_motor_set_constant_settings(pbio_port_t port, pbio_motor_dir_t direction){
    pbio_error_t status = pbio_motor_status(port);
    if (status == PBIO_SUCCESS) {
        motor_settings[motorindex(port)].direction = direction;
    }
    return status;
}

pbio_error_t pbio_motor_set_variable_settings(pbio_port_t port, int16_t max_duty){
    pbio_error_t status = pbio_motor_status(port);
    if (max_duty < 0 || max_duty > PBIO_MAX_DUTY_HARD) {
        status = PBIO_ERROR_INVALID_ARG;
    }
    if (status == PBIO_SUCCESS) { 
        motor_settings[motorindex(port)].max_duty = max_duty;
    }
    return status;
}

pbio_error_t pbio_motor_status(pbio_port_t port) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motor_files[motorindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_coast(pbio_port_t port) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motor_files[motorindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    slow_write(port, "command", "stop");
    motor_files[motorindex(port)].coasting = true;    

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motor_files[motorindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    // If the motor is currently in coast mode, set it back to run-direct mode
    if (motor_files[motorindex(port)].coasting) {
        slow_write(port, "command", "run-direct");
        motor_files[motorindex(port)].coasting = false;
    }
    // Limit the duty cycle value
    int16_t limit = motor_settings[motorindex(port)].max_duty;
    if (duty_cycle > limit) {
        duty_cycle = limit;
    }
    if (duty_cycle < -limit) {
        duty_cycle = -limit;
    }
    // Flip sign if motor is inverted
    if (motor_settings[motorindex(port)].direction == PBIO_MOTOR_DIR_INVERTED){
        duty_cycle = -duty_cycle;
    }
    fseek(motor_files[motorindex(port)].f_duty, 0, SEEK_SET);
    fprintf(motor_files[motorindex(port)].f_duty, "%d", duty_cycle/100);
    fflush(motor_files[motorindex(port)].f_duty); 

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motor_files[motorindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    fseek(motor_files[motorindex(port)].f_encoder_count, 0, SEEK_SET);
    fscanf(motor_files[motorindex(port)].f_encoder_count, "%d", count);
    fflush(motor_files[motorindex(port)].f_encoder_count);
    if (motor_settings[motorindex(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *count = -*count;
    }    
    return PBIO_SUCCESS;    
}

pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motor_files[motorindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    fseek(motor_files[motorindex(port)].f_encoder_rate, 0, SEEK_SET);
    fscanf(motor_files[motorindex(port)].f_encoder_rate, "%d", rate);
    fflush(motor_files[motorindex(port)].f_encoder_rate);
    if (motor_settings[motorindex(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *rate = -*rate;
    }
    return PBIO_SUCCESS;    
}
