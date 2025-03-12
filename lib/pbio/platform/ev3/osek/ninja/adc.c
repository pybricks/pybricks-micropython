/**
*  \file adc.c
*    
*  \brief This file contains function definitions to talk to the ADC
* 
*  \author ev3ninja
*/

/* Include statements */
#include "adc.h"
#include "gpio.h"
#include "spi.h"

/**
* \brief Get the value currently meassured by the ADC for the specified channel
*
* \param channel - The channel to meassure (ranging from 0 to 15)
*
* \return The value currently meassured by the ADC (ranging from 0 to 4095)
**/
unsigned short adc_get (unsigned short channel) {
	return (spi_update((0x1840 | ((channel & 0x000F) << 7)))) & 0x0FFF;
}


/**
* \brief Initialize the ADC by providing it with power
*
* \return none
**/
void adc_init (void) {
	// Init ADC power pins
	gpio_init_pin(GPIO_PIN(6, 14)); // 5VONIGEN
	gpio_init_pin(GPIO_PIN(0, 6));  // ADCBATEN

	// Enable battery power on ADC
	GPIO_SET(GPIO_PIN(0, 6))  =  GPIO_MASK(GPIO_PIN(0, 6));
	GPIO_DIR(GPIO_PIN(0, 6)) &= ~GPIO_MASK(GPIO_PIN(0, 6));
}


