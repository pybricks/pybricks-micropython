#include "sys/clock.h"

void mp_hal_set_interrupt_char(int c);
#define mp_hal_ticks_ms clock_time
#define mp_hal_ticks_us clock_usecs
#define mp_hal_delay_us clock_delay_usec
#define mp_hal_ticks_cpu() 0
