/**
*  \file init.c
*    
*  \brief This code is required in order to initialize the leJOS driver including all modules properly.
* 
*  \author Tobias Schießl, ev3ninja
*/

/* Include statements */
#include "init.h"
#include "systick.h"
#include "i2c.h"
#include "ninja/spi.h"
#include "ninja/adc.h"
#include "ninja/gpio.h"
#include "ninja/led.h"
#include "ninja/motor.h"
#include "ninja/button.h"
#include "include/hw/soc_AM1808.h"
#include "interrupt.h"
#include "stdio.h"

/* Global variable declaration */
/**
* \brief A flag indicating if the AINTC (Interrupt Controller) is already initialized or not
*
* This is required since the function leJOS_init will be called twice in OSEK applications, but the systick and the AINTC should only be initialized once.
**/ 
volatile U8 is_AINTC_initialized = 0;
/**
* \brief Array storing information about the 4 sensor ports of the EV3
**/ 
sensor_port_info ports[] = {
    { GPIO_PIN(8, 10), GPIO_PIN(2,  2), GPIO_PIN(0,  2), GPIO_PIN(0, 15), GPIO_PIN(8, 11), 0x6, 0x5 },
    { GPIO_PIN(8, 12), GPIO_PIN(8, 15), GPIO_PIN(0, 14), GPIO_PIN(0, 13), GPIO_PIN(8, 14), 0x8, 0x7 },
    { GPIO_PIN(8,  9), GPIO_PIN(7, 11), GPIO_PIN(0, 12), GPIO_PIN(1, 14), GPIO_PIN(7,  9), 0xA, 0x9 },
    { GPIO_PIN(6,  4), GPIO_PIN(7,  8), GPIO_PIN(0,  1), GPIO_PIN(1, 15), GPIO_PIN(7, 10), 0xC, 0xB },
};

/* Local function declarations */
void interrupt_init(void);
void sensor_init(void);

/**
* \brief Initialize the AINTC by enabling normal interrupts (IRQ) and fast interrupts (FIQ) on all required levels
*
* \return none
**/
void interrupt_init(void) {
    /* Initialize AINTC - this should only be done once per application */
    IntAINTCInit();
    
    /* Enable IRQ for ARM (in CPSR)*/
    IntMasterIRQEnable();
  
    /* Enable AINTC interrupts in GER */
    IntGlobalEnable();

    /* Enable IRQ in AINTC */
    IntIRQEnable();
    
    IntMasterFIQEnable();
    IntFIQEnable();
}

/**
* \brief Initialize the sensor ports of the EV3 by setting the correct pin-multiplexing configuration
*
* \return none
**/
void sensor_init(void) {
    unsigned int i;
    for (i = 0; i < sizeof(ports) / sizeof(ports[0]); ++i)
    {
        gpio_init_inpin(ports[i].pin1);
        gpio_init_inpin(ports[i].pin2);
        gpio_init_outpin(ports[i].pin5);
        gpio_init_pin(ports[i].pin6);
        gpio_init_pin(ports[i].buffer);
    }

    /* Disable Pull-Up/Pull-Down Resistors */
    *((volatile unsigned int*)(SOC_SYSCFG_1_REGS + 0x0C)) &= ~0xFFFFFFFF;
}

/**
* \brief Initialize the leJOS driver and all required submodules
*
* Initialization contains: AINTC, systick, sensor ports, motor ports, SPI controller, ADC, hardware buttons, LEDs
*
* \return none
**/
void leJOS_init(void) {
    if (!is_AINTC_initialized) {
        // This should be done only once per application - if we use OSEK, leJOS_init will be called at least 2 times
        interrupt_init();
        systick_init();
        is_AINTC_initialized = 1;
    }
    sensor_init();
    spi_init();
    adc_init();
    motor_init();
    led_init();
    button_init();
	// Confirm that the initialization was successful by setting the LEDs to green
    led_set(LED_BOTH, LED_GREEN);
}
