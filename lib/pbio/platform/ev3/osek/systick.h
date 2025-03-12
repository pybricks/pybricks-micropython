/**
*  \file ev3/systick.h
*    
*  \brief This header contains function declarations required to manage the tick in milliseconds on the EV3.
*
*  The tick is provided by a hardware timer of the AM1808 SoC which will trigger an interrupt every millisecond.
* 
*  \author Tobias Schießl
*/

#ifndef SYSTICK_H
#define SYSTICK_H

/* Include statements */
#include "mytypes.h"

void 	systick_init	(void);
U32 	systick_get_ms	(void);
void 	systick_wait_ms	(U32 ms);
void 	systick_wait_ns	(U32 n);
void 	systick_suspend	(void);
void 	systick_resume	(void);

#endif // SYSTICK_H