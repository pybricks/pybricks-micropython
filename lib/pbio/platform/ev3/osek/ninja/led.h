/**
*  \file led.h
*    
*  \brief This header declares function required to interact with the LEDs of the EV3 as well as enumerations therefore
* 
*  \author ev3ninja
*/

#pragma once

/**
* \brief Enumeration to represent the ID of the LEDs
*
* Every LED in fact consists of two LEDs, a red ond and a green one.
*/
typedef enum led_id {
	LED_LEFT   = 0x01,					///< The left LEDs (red and green) of the EV3
	LED_RIGHT  = 0x02,					///< The right LEDs (red and green) of the EV3
	LED_BOTH   = LED_LEFT | LED_RIGHT   ///< Both LEDs of the EV3
} led_id;

/**
* \brief Enumeration to represent the color which should be set for an LED
*/
typedef enum led_color {
	LED_BLACK  = 0x00,					///< Black color (which equals off)
	LED_RED    = 0x01,					///< Red color (which equals enabling only one GPIO pin)
	LED_GREEN  = 0x02,					///< Green color (which equals enabling only one GPIO pin)
	LED_ORANGE = LED_RED | LED_GREEN	///< Orange color (which equals enabling both GPIO pins)
} led_color;

/**
* \brief This struct stores information about one LED which is controlled by 2 GPIO pins.
*
* One GPIO pin controlls the red LED and one pin the green LED.
*/
typedef struct led_info {
	unsigned int pin1;					///< The first GPIO pin, responsible for the red LED
	unsigned int pin2;					///< The second GPIO pin, responsible for the green LED
} led_info;

/* LED interface */
void led_set	(led_id led, led_color color);
void led_init 	(void);