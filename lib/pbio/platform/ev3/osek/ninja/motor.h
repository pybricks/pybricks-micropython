/**
*  \file motor.h
*    
*  \brief A header file for motor driver. This file contains function declarations to use servo motors as well as enumerations and structs to represent motors and their states
* 
*  \author Bektur Marat uulu and Bektur Toktosunov 
*/

#pragma once

/* Include statements */
#include "mytypes.h"

/* Macro definitions */
/**
* \brief Number of output ports of servo motors.
**/ 
#define NO_OF_OUTPUT_PORTS 4

/* Struct definitions */
/**
 * \brief Contains the gpio pins of output and input pins.
 * 
 */
struct motor_port_info
{
    unsigned int pin1;      ///< Is used by pwm module to give power to motors
    unsigned int pin2;      ///< Is used by pwm module to give power to motors
    unsigned int pin5w;    
    unsigned int pin5r;     ///< INT value. Is used to track a wheel revolution of motors 
    unsigned int pin6;      ///< DIR value. Is used to track a wheel revolution of motors 
    unsigned short adc;
    //unsigned char adc1;  // adc channel 1.
    //unsigned char adc2;  // adc channel 2.
    //ninja adc channels: port1: 0x6, 0x5. port2: 0x8, 0x7. port3: 0xA, 0x9. port4: 0xC, 0xB
};
typedef struct motor_port_info motor_port_info;

/**
 * \brief Contains the information about wheel revolution, given speed and brake mode of motors
 */
struct motor_data_struct {
    int current_count; ///< current rotation angle in degrees
    int target_count; ///< target rotation angle to reach
    int speed_percent; ///< not used. for future use
    U32 last; ///< last value of INT (pin5)
    short brake_mode; ///<  true - brake immediately, false - float brake (not immediately).
};
typedef struct motor_data_struct motor_data_struct;

/* Enum definitions */
/**
 * \brief Enumeration of motor ports
 */
enum motor_port_id
{
  MOTOR_PORT_A = 0x00,
  MOTOR_PORT_B = 0x01,
  MOTOR_PORT_C = 0x02,
  MOTOR_PORT_D = 0x03
};
typedef enum motor_port_id motor_port_id;

/**
 * \brief Enumeration of motor types
 */
enum motor_type_id
{
  EV3_MEDIUM_MOTOR,
  NXT_SERVO_MOTOR,
  UNKNOWN_MOTOR_TYPE
};
typedef enum motor_type_id motor_type_id;

/**
 * \brief Enumeration of motor states
 */
enum motor_state
{
  MOTOR_BACKWARD  = 0x00,
  MOTOR_OFF       = 0x01,
  MOTOR_FORWARD   = 0x02
};
typedef enum motor_state motor_state;


void motor_set_state(motor_port_id port, motor_state state);

void set_duty_ma(U32 duty);
void set_duty_mb(U32 duty);
void set_duty_mc(U32 duty);
void set_duty_md(U32 duty);

unsigned int get_tacho_dir(motor_port_id Port);
unsigned int get_tacho_int(motor_port_id Port);
motor_type_id get_motor_type(motor_port_id Port);

void set_power(motor_port_id Port, S32 Power);

void motor_init (void);

int ev3_get_count(int motor_port_id);

void ev3_set_count(int motor_port_id, int count);

void ev3_motor_command(U32 motor_port_id, int cmd, int target_count, int speed_percent);

void ev3_motor_quad_decode(int motor_port_id);

void set_brake_mode(int motor_port_id, int brake_mode);
