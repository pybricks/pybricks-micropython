/**
*  \file ninja/gpio.h
*    
*  \brief This header declares function required to interact with GPIO pins of the SoC
* 
*  \author Tobias Schießl, ev3ninja
*/

#pragma once

/* Macro definitions */
/**
* \brief The base address of the SYSCFG0 register
**/ 
#define SYSCFG0_BASE      ((volatile void*)0x01C14000)
/**
* \brief The base address of the SYSCFG1 register
**/ 
#define SYSCFG1_BASE      ((volatile void*)0x01E2C000) 
/**
* \brief The base address of the GPIO control registers
**/ 
#define GPIO_BASE         ((volatile void*)0x01E26000)
/**
* \brief Calculate the number of a GPIO pin (0 to 143) based on its bank and number on that bank
**/ 
#define GPIO_PIN(B,O)     ((B) * 0x10 + (O))
/**
* \brief Get the base address of the registers responsible for controlling a specific GPIO pin based on the pin number
**/ 
#define GPIO_BANK(N)      (GPIO_BASE + 0x10 + (N >> 5) * 0x28)
/**
* \brief Get the mask required to manipulate the registers for the GPIO pin based on its pin number
**/ 
#define GPIO_MASK(N)      (1 << (N & 0x1F))

// prevent compiler warnings because GPIO_DIR gets redefined
#include "../include/hw/hw_gpio.h" 
#undef GPIO_DIR

/**
* \brief A dereferenced pointer to the GPIO direction register for a pin based on the pin number
**/ 
#define GPIO_DIR(N)       *((volatile unsigned int*)(GPIO_BANK(N) + 0x00))
/**
* \brief A dereferenced pointer to the GPIO set register for a pin based on the pin number
**/ 
#define GPIO_SET(N)       *((volatile unsigned int*)(GPIO_BANK(N) + 0x08))
/**
* \brief A dereferenced pointer to the GPIO clear register for a pin based on the pin number
**/ 
#define GPIO_CLR(N)       *((volatile unsigned int*)(GPIO_BANK(N) + 0x0C))

/* GPIO interface */
void 			gpio_init_pin		(unsigned int pin);
void 			gpio_init_outpin	(unsigned int pin);
void 			gpio_init_inpin		(unsigned int pin);
void 			gpio_set			(unsigned int pin, unsigned int value);
unsigned int 	gpio_get			(unsigned int pin);
void 			spi_init_pin		(unsigned int pin);
void 			turn_off_brick		(void);