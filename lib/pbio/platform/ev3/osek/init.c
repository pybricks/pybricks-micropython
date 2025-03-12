/**
*  \file init.c
*    
*  \brief This code is required in order to initialize the leJOS driver including all modules properly.
* 
*  \author Tobias Schießl, ev3ninja
*/

/* Include statements */
#include "init.h"
#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/armv5/am1808/interrupt.h>
#include "stdio.h"

/* Global variable declaration */
/**
* \brief A flag indicating if the AINTC (Interrupt Controller) is already initialized or not
*
* This is required since the function leJOS_init will be called twice in OSEK applications, but the systick and the AINTC should only be initialized once.
**/ 
volatile uint8_t is_AINTC_initialized = 0;

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
        // systick_init();
        is_AINTC_initialized = 1;
    }
}
