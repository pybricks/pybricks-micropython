/**
*  \file digi_sensors.h
*    
*  \brief Function declarations to talk to the digital sensors (I2C sensors)
* 
*  \author Tobias Schießl, Christian Soward
*/

#ifndef DIGI_SENSORS_H
#define DIGI_SENSORS_H

/* Include statements */
#include "sensors.h"

/* Macro definitions */
/*Ultrasonic sensor modes */
/**
* \brief Ultrasonic sensor mode "CONTINOUS" (sensor will meassure periodically by itself)
**/ 
#define ULTRASONIC_MODE_CONTINOUS   0x02
/**
* \brief Ultrasonic sensor mode "SINGLE SHOT" (sensor will meassure whenever a new single shot command is received and it will meassure up to 8 objects in front of it)
**/ 
#define ULTRASONIC_MODE_SINGLE_SHOT 0x01
/**
* \brief Ultrasonic sensor mode "OFF" (sensor will not meassure distances)
**/ 
#define ULTRASONIC_MODE_OFF         0x00
/* HiTechnic color sensor calibration modes */
/**
* \brief Constant value to send to the HiTechnic color sensor in order to start white calibration
**/ 
#define CAL_WHITE  					0x43
/**
* \brief Constant value to send to the HiTechnic color sensor in order to start black calibration
**/ 
#define CAL_BLACK  					0x42
/* HiTechnic color sensor color IDs */
/**
* \brief HiTechnic color sensor ID for black
**/ 
#define HI_TECHNIC_COL_BLACK        0
/**
* \brief HiTechnic color sensor ID for black
**/ 
#define HI_TECHNIC_COL_PURPLE       1
/**
* \brief HiTechnic color sensor ID for purple
**/ 
#define HI_TECHNIC_COL_DARK_BLUE    2
/**
* \brief HiTechnic color sensor ID for dark blue
**/ 
#define HI_TECHNIC_COL_LIGHT_BLUE   3
/**
* \brief HiTechnic color sensor ID for light blue
**/ 
#define HI_TECHNIC_COL_GREEN        4
/**
* \brief HiTechnic color sensor ID for green
**/ 
#define HI_TECHNIC_COL_YELLOW_GREEN 5
/**
* \brief HiTechnic color sensor ID for yellow green
**/ 
#define HI_TECHNIC_COL_YELLOW       6
/**
* \brief HiTechnic color sensor ID for yellow
**/ 
#define HI_TECHNIC_COL_ORANGE       7
/**
* \brief HiTechnic color sensor ID for orange
**/ 
#define HI_TECHNIC_COL_RED          8
/**
* \brief HiTechnic color sensor ID for red
**/ 
#define HI_TECHNIC_COL_RED_PINK     9
/**
* \brief HiTechnic color sensor ID for red pink
**/ 
#define HI_TECHNIC_COL_PINK         10
/**
* \brief HiTechnic color sensor ID for grey
**/ 
#define HI_TECHNIC_COL_GREY1        11
/**
* \brief HiTechnic color sensor ID for grey
**/ 
#define HI_TECHNIC_COL_GREY2        12
/**
* \brief HiTechnic color sensor ID for grey
**/ 
#define HI_TECHNIC_COL_GREY3        13
/**
* \brief HiTechnic color sensor ID for grey
**/ 
#define HI_TECHNIC_COL_GREY4        14
/**
* \brief HiTechnic color sensor ID for grey
**/ 
#define HI_TECHNIC_COL_GREY5        15
/**
* \brief HiTechnic color sensor ID for grey
**/ 
#define HI_TECHNIC_COL_GREY6        16
/**
* \brief HiTechnic color sensor ID for white
**/ 
#define HI_TECHNIC_COL_WHITE        17
/* HiTechnic temperature sensor resolution modes */
/**
* \brief HiTechnic temperature sensor mode with 9 bit resolution (0.5 degrees C). Average Conversion time within sensor in this mode is 27.5ms
**/
#define TEMP9BIT 0x00
/**
* \brief HiTechnic temperature sensor mode with 10 bit resolution (0.25 degrees C). Average Conversion time within sensor in this mode is 55ms
**/
#define TEMP10BIT 0x01
/**
* \brief HiTechnic temperature sensor mode with 11 bit resolution (0.125 degrees C). Average Conversion time within sensor in this mode is 110ms
**/
#define TEMP11BIT 0x02
/**
* \brief HiTechnic temperature sensor mode with 12 bit resolution (0.0625 degrees C). Average Conversion time within sensor in this mode is 220ms
**/
#define TEMP12BIT 0x03

void sensor_digi_enable(int port);

/* Lego ultrasonic sensor API */
void 			sensor_ultrasonic_get_range				(int port, unsigned char data_buffer[8]);
void 			sensor_ultrasonic_set_mode				(int port, unsigned char mode);

/* HiTechnic compass sensor API */
unsigned short 	sensor_compass_get						(int port);
void 			sensor_compass_start_calibration		(int port);
unsigned short 	sensor_compass_end_calibration			(int port);

/* HiTechnic acceleration sensor API */
void 			sensor_accel_calibrate					(int port);
void 			sensor_accel_get						(int port, short data_buffer[3]);
void 			sensor_accel_get_raw					(int port, short data_buffer[3]);

/* HiTechnic color sensor API */
unsigned char 	sensor_hitechnic_color_calibrate		(int port, int mode);
void 			sensor_hitechnic_color_get				(int port, signed short data_buffer[3]);
unsigned char 	sensor_hitechnic_color_get_color_id		(int port);
void 			sensor_hitechnic_color_id_to_string		(unsigned char color_id, char *string_buffer);

/* Lego temperature sensor API */
void 			sensor_temperature_nxt_set_resolution	(int port, unsigned char resolution);
unsigned char 	sensor_temperature_nxt_get_resolution	(int port);
float 			sensor_temperature_nxt_get_temperature	(int port);

#endif // DIGI_SENSORS_H
