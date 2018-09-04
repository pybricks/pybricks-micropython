
#include <stdbool.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <pbio/motor.h>


#define MAX_PATH_LENGTH 50
#define MAX_DUTY_HARD 100

// Index of a pbio_port_t type
#define pindex(port)  (port-PBIO_PORT_A)

// Motor file structure for each motor
typedef struct _motorfile_t {
    bool connected;
    bool coasting;
    int dir_number;
    FILE *f_encoder_count;
    FILE *f_encoder_rate;
    FILE *f_duty;
} motorfile_t;

motorfile_t motorfiles[pindex(PBIO_PORT_D)+1] = {[pindex(PBIO_PORT_A) ... pindex(PBIO_PORT_D)].connected=false};

// Open file, write contents, and close it
void slow_write(pbio_port_t port, const char* filename, const char* content) {
    // Open the file in the directory corresponding to the specified port
    char filepath[MAX_PATH_LENGTH];
    sprintf(filepath, "/sys/class/tacho-motor/motor%d/%s", motorfiles[pindex(port)].dir_number, filename);
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
            sprintf(ppath, "/sys/class/tacho-motor/motor%d/address", dir_number);
            FILE* addr = fopen(ppath, "r");
            // Extract the port (14th) character, convert to numeric, and close address file
            fseek(addr, 13, SEEK_SET);
            port_index = fgetc(addr) - 'A';
            fclose(addr);
            // Store that this motor is present and save its location
            motorfiles[port_index].connected = true;
            motorfiles[port_index].dir_number = dir_number;
        }
    }
    // Close tacho-motor directory when done
    closedir (dp);

    // Now that we know which motors are present, open the relevant files for reading and writing
    for(pbio_port_t port = PBIO_PORT_A; port < PBIO_PORT_D; port++) {
        int port_index = pindex(port);
        if (motorfiles[port_index].connected) {
            //Debug message. Should replace with debug print
            printf("Detected motor%d on port %c.\n", motorfiles[port_index].dir_number, port_index+65);
            // Reset the motor and let stop default to coast
            slow_write(port, "command", "reset");
            slow_write(port, "stop_action", "coast");
            pbio_motor_coast(port);
            // File path character array to the relevant speed, position files, etc.
            char filepath[MAX_PATH_LENGTH];
            // Open the position file
            sprintf(filepath, "/sys/class/tacho-motor/motor%d/position", motorfiles[port_index].dir_number);
            motorfiles[port_index].f_encoder_count = fopen(filepath, "r");
            // Open the speed file
            sprintf(filepath, "/sys/class/tacho-motor/motor%d/speed", motorfiles[port_index].dir_number);
            motorfiles[port_index].f_encoder_rate = fopen(filepath, "r");            
            // Open the duty file
            sprintf(filepath, "/sys/class/tacho-motor/motor%d/duty_cycle_sp", motorfiles[port_index].dir_number);
            motorfiles[port_index].f_duty = fopen(filepath, "w");
        }
    }    
}

void pbio_motor_deinit(void) {
    // Close the relevant files
    for(pbio_port_t port = PBIO_PORT_A; port < PBIO_PORT_D; port++) {
        int port_index = pindex(port);
        if (motorfiles[port_index].connected) {
            // Only close files for motors that are attached
            fclose(motorfiles[port_index].f_encoder_count);
            fclose(motorfiles[port_index].f_encoder_rate);
            fclose(motorfiles[port_index].f_duty);
            // Reset the motor
            slow_write(port, "command", "reset");
        }
    }
}

pbio_error_t pbio_motor_coast(pbio_port_t port) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motorfiles[pindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    slow_write(port, "command", "stop");
    motorfiles[pindex(port)].coasting = true;    

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_set_duty_cycle(pbio_port_t port, int16_t duty_cycle) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motorfiles[pindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    // If the motor is currently in coast mode, set it back to run-direct mode
    if (motorfiles[pindex(port)].coasting) {
        slow_write(port, "command", "run-direct");
        motorfiles[pindex(port)].coasting = false;
    }
    // Limit the duty cycle value
    if (duty_cycle > MAX_DUTY_HARD) {
        duty_cycle = MAX_DUTY_HARD;
    }
    if (duty_cycle < -MAX_DUTY_HARD) {
        duty_cycle = -MAX_DUTY_HARD;
    }    
    // Set the duty cycle value, limited by the hard constraint of the device
    fseek(motorfiles[pindex(port)].f_duty, 0, SEEK_SET);
    fprintf(motorfiles[pindex(port)].f_duty, "%d", duty_cycle);
    fflush(motorfiles[pindex(port)].f_duty); 

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_set_direction(pbio_port_t port, pbio_motor_dir_t direction) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motorfiles[pindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    if (direction == PBIO_MOTOR_DIR_INVERTED) {
        slow_write(port, "polarity", "inversed");
    }
    else{
        slow_write(port, "polarity", "normal");
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motorfiles[pindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    fseek(motorfiles[pindex(port)].f_encoder_count, 0, SEEK_SET);
    fscanf(motorfiles[pindex(port)].f_encoder_count, "%d", count);
    fflush(motorfiles[pindex(port)].f_encoder_count);

    return PBIO_SUCCESS;    
}

pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    if (port < PBIO_PORT_A || port > PBIO_PORT_D) {
        return PBIO_ERROR_INVALID_PORT;
    }
    if (!motorfiles[pindex(port)].connected) {
        return PBIO_ERROR_NO_DEV;
    }
    fseek(motorfiles[pindex(port)].f_encoder_rate, 0, SEEK_SET);
    fscanf(motorfiles[pindex(port)].f_encoder_rate, "%d", rate);
    fflush(motorfiles[pindex(port)].f_encoder_rate);

    return PBIO_SUCCESS;    
}
