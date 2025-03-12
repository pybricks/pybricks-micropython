/**
*  \file ev3_motors.c
*    
*  \brief This file contains function definitions to use EV3 motors. Driver wrapper for ECRobot API.
* 
*  \author
*/

/* Include statements */
#include "ev3_motors.h"
#include "ninja/motor.h"
#include "stdio.h"

/**
* \brief Get motor revolution count in degree
* 
* \param n - Motor port id 
*
* \return revolution in degree
**/
int nxt_motor_get_count(U32 n) {
    return ev3_get_count(n);
}

/**
* \brief Set motor revolution count in degree
* 
* \param n - Motor port id 
* \param count - Motor revolution count to set
*
* \return none
**/
void nxt_motor_set_count(U32 n, int count) {
    ev3_set_count(n, count);
}

/**
* \brief Set motor speed and brake mode
* 
* \param n - Motor port id 
* \param speed_percent - Speed percent. The given motor rotates backward if negative
* \param brake - Brake mode. True - brake (stop immediately), false - float (soft stop) 
*
* \return none
**/
void nxt_motor_set_speed(U32 n, int speed_percent, int brake) {
    set_power((motor_port_id) n, speed_percent);
    set_brake_mode((motor_port_id) n, brake);
}

/**
* \brief Set motor target revolution count to reach, speed percent and brake mode
* 
* \param n - Motor port id 
* \param cmd - Brake mode. True - brake, false - stop
* \param target_count - Target revolution count to reach and stop
* \param speed_percent - Speed percent to rotate
*
* \return none
**/
void nxt_motor_command(U32 n, int cmd, int target_count, int speed_percent) {
    ev3_motor_command(n, cmd, target_count, speed_percent);
}
