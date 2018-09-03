/*

   ev3dev specific implementation of the low level motor commands.

*/

#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <string.h>

#define MAX_DUTY_HARD 100
#define N_OUTPUTS 4
#define PORT_A (0x00)
#define PORT_B (0x01)
#define PORT_C (0x02)
#define PORT_D (0x03)

// (de)Initialize all motors that are currently connected
void motor_init();
void motor_deinit();

// Set positive rotation direction
void set_positive_direction(int port, bool clockwise);

// Get the encoder of the motor
int get_encoder(int port);

// Get the encoder rate of the motor
int get_rate(int port);

// Set motor to coast mode
void set_coast(int port);

// Set duty percentage of motor
void set_duty(int port, int duty);
