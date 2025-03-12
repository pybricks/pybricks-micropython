/**
*  \file init.h
*    
*  \brief This header provides the interface to initialize the leJOS EV3 drivers.
* 
*  \author Tobias Schießl, ev3ninja
*/

#ifndef INIT_H
#define INIT_H

/* Struct definitions */
/**
* \brief This struct combines information about a sensor port of the EV3.
* 
* It is based on code taken from the ev3ninja project. Each sensor port is described by 4 GPIO pins which connect to the sensor, 1 buffer GPIO pin and 2 ADC channels.
*/
typedef struct sensor_port_info {
    unsigned int pin1;      ///< I_ONB - 9V enable (high)
    unsigned int pin2;      ///< LEGDETB - Digital input pulled up
    unsigned int pin5;      ///< DIGIB0 - Digital input/output
    unsigned int pin6;      ///< DIGIB1 - Digital input/output
    unsigned int buffer;    ///< Buffer disable
    unsigned char adc1;     ///< ADC channel 1
    unsigned char adc2;     ///< ADC channel 2
} sensor_port_info;

void leJOS_init(void);

#endif // INIT_H