/**
*  \file adc_sensors.c
*    
*  \brief This file contains the function definitions to read the analogous sensors
* 
*  \author Tobias Schießl
*/

/* Include statements */
#include "ninja/adc.h"
#include "ninja/gpio.h"
#include "init.h"
#include "stdio.h"  

/* Macro definitions */
/**
* \brief The 10 bit value the gyro sensor will return if in rest
**/ 
#define GYRO_SENSOR_OFFSET  (2444 >> 2)

/* Global variable definitions */
/**
* \brief Array storing the information if an analogous sensor is red for the first time or not
**/ 
unsigned char first[4] = {1, 1, 1, 1};
/**
* \brief Array storing the information if a sound sensor is initialized or not
**/ 
unsigned char sound_sensors_initialized[4] = {0, 0, 0, 0};

/* External variables */
extern sensor_port_info ports[];

/* Local function declarations */
void check_first(int port);

/**
* \brief Check if we read an analogous sensor for the first time and read the current value if that's the case
*
* \param port - The port the sensor is connected to
*
* \return none
**/
void check_first(int port) {
    if (first[port] == 1) {
        adc_get(ports[port].adc1); 
        adc_get(ports[port].adc2); 
        first[port] = 0;
    }
}

/**
* \brief Read the state of the NXT touch sensor connected to the specified port
*
* If the sensor is red for the first time, the first value is invalid and will be discarded. The current state is red a second time in that case.
*
* \param port - The port the sensor is connected to
*
* \return The current state of the touch sensor: 1 if it is pressed, 0 otherwise
**/
int sensor_touch_get_state(int port) {
    check_first(port);
    unsigned short Data1 = adc_get(ports[port].adc1);
    unsigned short Data2 = adc_get(ports[port].adc2);

    return ((Data2 > 3000) || (Data1 > 500 && Data1 < 1000) ? 1 : 0);
}

/**
* \brief Set an NXT light sensor at the specified port active by enabling its LED
*
* \param port - The port the sensor is connected to
*
* \return none
**/
void sensor_light_set_active(int port) {
    // Turn the light on
    gpio_set(ports[port].pin5, 1);
}

/**
* \brief Set an NXT light sensor at the specified port inactive by disabling its LED
*
* \param port - The port the sensor is connected to
*
* \return none
**/
void sensor_light_set_inactive(int port) {
    // Turn the light off
    gpio_set(ports[port].pin5, 0);
}

/**
* \brief Get the 10 bit value of the NXT light sensor at the specified port
*
* \param port - The port the sensor is connected to
*
* \return The current value of the light sensor, ranging from 0 (White) to 1023 (Black)
**/
unsigned short sensor_light_get(int port) {
    unsigned short light_sensor_value = adc_get(ports[port].adc1);
    light_sensor_value = light_sensor_value >> 2;
    return light_sensor_value;
}

/**
* \brief Get the 10 bit value of the NXT sound sensor at the specified port
*
* If this function is called for the first time with the specified port, the sound sensor will be initialized by setting its mode to dB.
*
* \param port - The port the sensor is connected to
*
* \return The current value of the sound sensor, ranging from 0 (loud) to 1023 (quiet)
**/
unsigned short sensor_sound_get(int port) {
    if (sound_sensors_initialized[port] == 0) {
        // Selects Mode dB or dBA (not sure yet) - this mode works better
        gpio_set(ports[port].pin5, 0);
        gpio_set(ports[port].pin6, 1);
    }
    unsigned short sound_sensor_value = adc_get(ports[port].adc1);
    sound_sensor_value = sound_sensor_value >> 2;
    return sound_sensor_value;
}

/**
* \brief Get the current rotation meassured by the HiTechnic gyro sensor connected at the specified port
*
* The value returned by the sensor will be transformed into the range from -360 degrees  to +360 degrees.
*
* \param port - The port the sensor is connected to
*
* \return The current rotation meassured by the gyro sensor, ranging from -360 degrees  to +360 degrees
**/
signed short sensor_gyro_get(int port) {
    signed short gyro_value = adc_get(ports[port].adc1) - GYRO_SENSOR_OFFSET;
    if (gyro_value >= 0)
        gyro_value = 360 * (((float) gyro_value) / (0x3FF - GYRO_SENSOR_OFFSET));
    else
        gyro_value = 360 * (((float) gyro_value) / GYRO_SENSOR_OFFSET);
    return gyro_value;
}