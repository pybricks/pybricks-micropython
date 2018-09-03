#include "motorio.h"

#define MAX_PATH_LENGTH 50

// Motor file structure for each motor
typedef struct _motorfile_t {
    bool connected;
    bool coasting;
    int dir_number;
    FILE *f_position;
    FILE *f_speed;
    FILE *f_duty;
} motorfile_t;

motorfile_t motorfiles[N_OUTPUTS] = {[0 ... N_OUTPUTS-1].connected=false};

// Open file, write contents, and close it
void slow_write(int port, const char* filename, const char* content){
    // Open the file in the directory corresponding to the specified port
    char filepath[MAX_PATH_LENGTH];
    sprintf(filepath, "/sys/class/tacho-motor/motor%d/%s", motorfiles[port].dir_number, filename);
    FILE* file = fopen(filepath, "w"); 
    // Write the contents to the file
    fprintf(file, content);  
    // Close the file
    fclose(file);            
}

// Read the position of the motor
int get_encoder(int port){
    if(motorfiles[port].connected){
        int position;
        fseek(motorfiles[port].f_position, 0, SEEK_SET);
        fscanf(motorfiles[port].f_position, "%d", &position);
        fflush(motorfiles[port].f_position);
        return position;
    }
    else{
        return 0;
    }
}

// Read the speed of the motor
int get_rate(int port){
    if(motorfiles[port].connected){
        int speed;
        fseek(motorfiles[port].f_speed, 0, SEEK_SET);
        fscanf(motorfiles[port].f_speed, "%d", &speed);
        fflush(motorfiles[port].f_speed);
        return speed;
    }
    else{
        return 0;
    }
}

// Set the motor to coast mode
void set_coast(int port){
    if(motorfiles[port].connected){       
        slow_write(port, "command", "stop");
        motorfiles[port].coasting = true;
    }
}

// Set duty cycle
void set_duty(int port, int duty){
    if(motorfiles[port].connected){   
        // If the motor is currently in coast mode, set it back to run-direct mode
        if(motorfiles[port].coasting){
            slow_write(port, "command", "run-direct");
            motorfiles[port].coasting = false;
        }
        // Limit the duty cycle value
        if(duty > MAX_DUTY_HARD){
            duty = MAX_DUTY_HARD;
        }
        if(duty < -MAX_DUTY_HARD){
            duty = -MAX_DUTY_HARD;
        }    
        // Set the duty cycle value, limited by the hard constraint of the device
        fseek(motorfiles[port].f_duty, 0, SEEK_SET);
        fprintf(motorfiles[port].f_duty, "%d", duty);
        fflush(motorfiles[port].f_duty);
    }
}

// Set forward direction
void set_positive_direction(int port, bool clockwise){
    if(motorfiles[port].connected){
        if(clockwise){
            slow_write(port, "polarity", "normal");
        }
        else{
            slow_write(port, "polarity", "inversed");
        }
    }    
}

// Initialize the motors
void motor_init(){
    // Open tacho-motor directory
    DIR *dp;
    struct dirent *ep;
    dp = opendir ("/sys/class/tacho-motor");
    // Loop through the motorXs
    while ((ep = readdir(dp))){
        // Ignore the . and .. folders
        if (strlen(ep->d_name) > 5){
            // Obtain the numeric device ID from the path
            int dir_number, port_number;
            sscanf(ep->d_name, "%*5c%d",&dir_number);
            // Open the address file in this folder
            char ppath[MAX_PATH_LENGTH];
            sprintf(ppath, "/sys/class/tacho-motor/motor%d/address", dir_number);
            FILE* addr = fopen(ppath, "r");
            // Extract the port (14th) character, convert to numeric, and close address file
            fseek(addr, 13, SEEK_SET);
            port_number = fgetc(addr) - 'A';
            fclose(addr);
            // Store that this motor is present and save its location
            motorfiles[port_number].connected = true;
            motorfiles[port_number].dir_number = dir_number;
        }
    }
    // Close tacho-motor directory when done
    closedir (dp);

    // Now that we know which motors are present, open the relevant files for reading and writing
    for(int port = 0; port < N_OUTPUTS; port++){
        if (motorfiles[port].connected){
            // Debug message. Should replace with debug print
            // printf("Detected motor%d on port %c.\n", motorfiles[port].dir_number, port+65);
            // Reset the motor and let stop default to coast
            slow_write(port, "command", "reset");
            slow_write(port, "stop_action", "coast");
            set_coast(port);
            // File path character array to the relevant speed, position files, etc.
            char filepath[MAX_PATH_LENGTH];
            // Open the position file
            sprintf(filepath, "/sys/class/tacho-motor/motor%d/position", motorfiles[port].dir_number);
            motorfiles[port].f_position = fopen(filepath, "r");
            // Open the speed file
            sprintf(filepath, "/sys/class/tacho-motor/motor%d/speed", motorfiles[port].dir_number);
            motorfiles[port].f_speed = fopen(filepath, "r");            
            // Open the duty file
            sprintf(filepath, "/sys/class/tacho-motor/motor%d/duty_cycle_sp", motorfiles[port].dir_number);
            motorfiles[port].f_duty = fopen(filepath, "w");
        }
    }
}

void motor_deinit(){
    // Close the relevant files
    for(int port = 0; port < N_OUTPUTS; port++){
        if (motorfiles[port].connected){
            // Only close files for motors that are attached
            fclose(motorfiles[port].f_position);
            fclose(motorfiles[port].f_speed);
            fclose(motorfiles[port].f_duty);
            // Reset the motor
            slow_write(port, "command", "reset");
        }
    }
}

