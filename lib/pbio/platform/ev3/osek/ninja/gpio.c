/**
*  \file ninja/gpio.c
*    
*  \brief This contains function definitions required to interact with GPIO pins of the SoC
* 
*  \author Tobias Schießl, ev3ninja
*/

/* Include statements */
#include "stdio.h"
#include "gpio.h"
#include "pininfo.h"

/* Macro definitions */
/**
* \brief The KICK registers required to unlock access to some SYSCFG register (including the pin-multiplexing registers)
**/ 
#define SYSCFG_KICK(N)    (*((volatile unsigned int*)(SYSCFG0_BASE + 0x38 + (N) * 0x4)))
/**
* \brief The PINMUX registers in the SYSCFG0 register in order to manipulate the pin-multiplexing configuration
**/ 
#define SYSCFG_PINMUX(N)  (*((volatile unsigned int*)(SYSCFG0_BASE + 0x120 + (N) * 0x4)))
/**
* \brief The value which needs to be written to KICK0 register in order to unlock the protected registers
**/ 
#define KICK0_UNLOCK      0x83E70B13
/**
* \brief The value which needs to be written to KICK1 register in order to unlock the protected registers
**/ 
#define KICK1_UNLOCK      0x95A4F1E0
/**
* \brief Any random value which can to be written to KICK0 register in order to lock the protected registers
**/ 
#define KICK0_LOCK        0x0
/**
* \brief Any random value which can to be written to KICK1 register in order to lock the protected registers
**/ 
#define KICK1_LOCK        0x0
/**
* \brief Unlock the protected registers by writing the required values into the KICK registers
**/ 
#define SYSCFG_UNLOCK     { SYSCFG_KICK(0) = KICK0_UNLOCK; SYSCFG_KICK(1) = KICK1_UNLOCK; }
/**
* \brief Lock the protected registers by writing random values into the KICK registers
**/ 
#define SYSCFG_LOCK       { SYSCFG_KICK(0) = KICK0_LOCK; SYSCFG_KICK(1) = KICK1_LOCK; }

/**
* \brief Initialize a GPIO pin with its GPIO functionality
*
* \param pin - The pin number of the GPIO pin , ranging from 0 to 143)
*
* \return none
**/
void gpio_init_pin (unsigned int pin) {
	SYSCFG_UNLOCK;

	// Setup pin multiplexing
	pin_info pi = pininfo[pin];

	if (__builtin_expect(pin >= pininfo_size || pi.muxreg_mask == 0, 0))
		printf("gpio: can not initialize pin %x - need init information in pin_info\n", pin);

	SYSCFG_PINMUX(pi.muxreg) &= pi.muxreg_mask;
	SYSCFG_PINMUX(pi.muxreg) |= pi.muxreg_mode;

	SYSCFG_LOCK;
}

/**
* \brief Initialize a GPIO pin with its GPIO functionality and set its direction to output
*
* \param pin - The pin number of the GPIO pin , ranging from 0 to 143)
*
* \return none
**/
void gpio_init_outpin (unsigned int pin) {
	SYSCFG_UNLOCK;

	gpio_init_pin(pin);

	// Clear pin data and set direction
	GPIO_CLR(pin)  =  GPIO_MASK(pin);
	GPIO_DIR(pin) &= ~GPIO_MASK(pin);

	SYSCFG_LOCK;
}

/**
* \brief Initialize a GPIO pin with its GPIO functionality and set its direction to input
*
* \param pin - The pin number of the GPIO pin , ranging from 0 to 143)
*
* \return none
**/
void gpio_init_inpin (unsigned int pin) {
	SYSCFG_UNLOCK;

	gpio_init_pin(pin);

	// Set direction
	GPIO_DIR(pin) |=  GPIO_MASK(pin);

	SYSCFG_LOCK;
}

/**
* \brief Set the value of a GPIO pin which is configured as output
*
* If not already done, this function will set the pin as an output pin.
*
* \param pin - The pin number of the GPIO pin , ranging from 0 to 143)
* \param value - The value to set (0 will be low, all other values will map to high)
*
* \return none
**/
void gpio_set (unsigned int pin, unsigned int value) {
	gpio_init_outpin(pin);
	*((volatile unsigned int*)(GPIO_BANK(pin) + 8 + (!value) * 4)) = GPIO_MASK(pin);
}

/**
* \brief Get the signal at a GPIO pin which is configured as input
*
* If not already done, this function will set the pin as an input pin.
*
* \param pin - The pin number of the GPIO pin , ranging from 0 to 143)
*
* \return The signal at the pin: 1 for high and 0 for low
**/
unsigned int gpio_get (unsigned int pin) {
	gpio_init_inpin(pin);
	volatile unsigned int *Reg = (GPIO_BANK(pin) + 0x10);
	return (((*Reg) & GPIO_MASK(pin)) != 0);
}

/**
* \brief Turn the EV3 off by setting GPIO pin 6 on bank 11 as an output pin
*
* \return none
**/
void turn_off_brick(void) {
	// GPIO 6-11
	gpio_init_outpin(GPIO_PIN(6, 11));
}