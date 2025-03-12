/**
*  \file ev3_motors.h
*    
*  \brief This header contains function declarations to use motor drivers. Driver wrapper for ECRobot API.
* 
*  \author
*/
#ifndef NXT_MOTORS_H
#define NXT_MOTORS_H

/* Include statements */
#include "mytypes.h"
#include "./ninja/motor.h"

/* Macro definitions */
#define EV3_N_MOTORS 3

#define NXT_PORT_A     MOTOR_PORT_A
#define NXT_PORT_B     MOTOR_PORT_B
#define NXT_PORT_C     MOTOR_PORT_C

#define EV3_PORT_A     MOTOR_PORT_A
#define EV3_PORT_B     MOTOR_PORT_B
#define EV3_PORT_C     MOTOR_PORT_C
#define EV3_PORT_D     MOTOR_PORT_D

int nxt_motor_get_count(U32 n);

void nxt_motor_set_count(U32 n, int count);

void nxt_motor_set_speed(U32 n, int speed_percent, int brake);

void nxt_motor_command(U32 n, int cmd, int target_count, int speed_percent);

#endif
