#include "stdio.h"
#include "hw_syscfg0_AM1808.h"
#include "evmAM1808.h"
#include "kernel.h"
#include "cpu_insn.h"
#include "button.h"
#include "power.h"
#include "armv5/am1808/interrupt.h"
#include "soc_AM1808.h"
#include "include/gpio.h"
#include "ninja/motor.h"
#include "ninja/gpio.h"

extern void ExceptionHandler(void);

void start_boot(void);
void copy_vector_table(void);
void check_EV3_buttons(void);

volatile unsigned long button_counter = 0;

void start_boot(void) {
    SysCfgRegistersUnlock();
    
    copy_vector_table();
    
    StartOS(1);
}

void copy_vector_table(void) {
    unsigned int *dest                  = (unsigned int *)  0xFFFF0000;
    unsigned int *addrExceptionHandler  = (unsigned int *)  ExceptionHandler;
    int i                               = 1;
    
    for (; i < 8 + 2048; ++i) {
        dest[i] = addrExceptionHandler[i];
    }
}

void check_EV3_buttons(void) {
    /* Fallback code
    button_id b = button_get_pressed();
    switch (b) {
        case BUTTON_BACK:
            power_off();
            break;
        default:
            // Nothing to do
            break;
    } 
    */
    
    // The following code used to crash the program - I could identify incrementing the variable as the problem source
    // For now it seems to work (maybe the problem was also stack related) - if we get problems again, use the fallback code above
    button_counter = (button_counter + 1) % 50; // We check the buttons every 50 ms
    if (!button_counter) {
        button_id button = button_get_pressed();
        switch (button) {
            case BUTTON_BACK:
                power_off();
                break;
            default:
                // Nothing to do
                break;
        }
    }
    
}


