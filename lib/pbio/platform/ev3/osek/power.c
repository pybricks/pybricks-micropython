/**
*  \file power.c
*    
*  \brief This header contains function definitions for the power management on the EV3.
* 
*  \author Tobias Schießl, Bektur Marat uulu, Bektur Toktosunov
*/

/* Include statements */
#include "power.h"
#include "ninja/adc.h"
#include "ninja/gpio.h"

/**
* \brief Turn of the EV3
* 
* \return none
*/
void power_off(void) {
    turn_off_brick();
}

/**
* \brief Get the voltage of the battery
* 
* \return The volatge of the battery
*/
U16 get_battery_voltage(void) {
    return adc_get((unsigned short) 4) * 2;
}

/**
* \brief Get the current of the battery
* 
* \return The current of the battery
*/
U16 get_battery_vurrent(void) {
    return adc_get((unsigned short) 3) / 15;
}