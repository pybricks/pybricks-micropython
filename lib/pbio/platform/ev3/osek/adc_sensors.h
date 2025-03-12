/**
*  \file adc_sensors.h
*    
*  \brief Function declarations to read the analogous sensors
* 
*  \author Tobias Schießl
*/

#ifndef ADC_SENSORS_H
#define ADC_SENSORS_H

/* Include statements */
#include "sensors.h"

/* NXT touch sensor API */
int 			sensor_touch_get_state		(int port);

/* NXT light sensor API */
void 			sensor_light_set_active		(int port);
void 			sensor_light_set_inactive	(int port);
unsigned short 	sensor_light_get		(int port);

/* NXT sound sensor API */
unsigned short 	sensor_sound_get		(int port);

/* HiTechnic gyro sensor API */
signed short 	sensor_gyro_get				(int port);

#endif //ADC_SENSORS_H