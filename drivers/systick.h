#ifndef __NXOS_SYSTICK_H__
#define __NXOS_SYSTICK_H__

#include "base/mytypes.h"

void nx_systick_init();
U32 nx_systick_get_ms();
void nx_systick_wait_ms(U32 ms);
void nx_systick_wait_ns(U32 ns);
void nx_systick_install_scheduler(nx_closure_t scheduler_cb);

#endif
