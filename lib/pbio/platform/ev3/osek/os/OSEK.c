#include "stdio.h"
#include "hw_syscfg0_AM1808.h"
#include "evmAM1808.h"
// #include "kernel.h"
// #include "cpu_insn.h"
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

extern void leJOS_init(void);

void user_1ms_isr_type2(void) {
    // SignalCounter(SysTimerCnt);
}
uint8_t		callevel;
static volatile unsigned long shouldDispatch = 0;
volatile unsigned int addrShouldDispatch = (unsigned int) &shouldDispatch;

void SetDispatch(void){
// 	if (runtsk != INVALID_TASK && runtsk != schedtsk){ // If runtsk is INVALID_TASK we are in idle mode and therefore will schedule automatically
// 		shouldDispatch = 1;
// 	} // Flag to indicate that after we return from the ISR we need to dispatch the current task (will be checked in IRQ Handler)
}

extern void SystemInit(void);

extern int main(int argc, char **argv);

void start_boot(void) {
    SysCfgRegistersUnlock();
    
    copy_vector_table();
    
    // StartOS(1);
    leJOS_init();

    SystemInit();
    main(0, NULL);
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


