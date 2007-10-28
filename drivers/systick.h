#ifndef __NXOS_SYSTICK_H__
#define __NXOS_SYSTICK_H__

#include "mytypes.h"

void systick_init();
U32 systick_get_ms();
void systick_wait_ms(U32 ms);
void systick_wait_ns(U32 ns);

#endif
