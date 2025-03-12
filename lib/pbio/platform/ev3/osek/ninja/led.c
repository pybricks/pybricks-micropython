/**
*  \file led.c
*    
*  \brief This contains function definitions required to interact with the LEDs of the EV3
* 
*  \author ev3ninja
*/

/* Include statements */
#include "led.h"
#include "gpio.h"

/* Global variable definitions */
/**
* \brief This array stores information about the 2 LEDs (left and right)
**/ 
led_info leds[] = {
	{ GPIO_PIN(6, 13), GPIO_PIN(6,  7) }, // LED_LEFT
	{ GPIO_PIN(6, 12), GPIO_PIN(6, 14) }  // LED_RIGHT
};

/**
* \brief Set the color of the given LED
*
* \param led - The ID of the LED to control
* \param color - The color to set
*
* \return none
**/
void led_set (led_id led, led_color color) {
	if (led & LED_LEFT) {
		gpio_set(leds[0].pin1, color & 1);
		gpio_set(leds[0].pin2, (color & 2) >> 1);
	}
	if (led & LED_RIGHT) {
		gpio_set(leds[1].pin1, color & 1);
		gpio_set(leds[1].pin2, (color & 2) >> 1);
	}
}

/**
* \brief Initialize the GPIO pins required to manipulate the LEDs
*
* \return none
**/
void led_init (void) {
	gpio_init_outpin(leds[0].pin1);
	gpio_init_outpin(leds[0].pin2);
	gpio_init_outpin(leds[1].pin1);
	gpio_init_outpin(leds[1].pin2);
}