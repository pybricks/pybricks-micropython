#include "stdio.h"
#include "hw_syscfg0_AM1808.h"
#include "evmAM1808.h"
#include "armv5/am1808/interrupt.h"
#include "soc_AM1808.h"

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


