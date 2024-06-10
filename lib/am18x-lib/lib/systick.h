// tary, 23:39 2011/6/24	      
#ifndef __SYSTICK_H__
#define __SYSTICK_H__

typedef int (*systick_handler_t)(int);

// period unit: milli seconds
int systick_init(unsigned period);
int systick_start(void);
int systick_stop(void);
int systick_set_handler(systick_handler_t handle);
int systick_timeout(int msecs);
int systick_sleep(int msecs);
// return unit: milli seconds
unsigned systick_period(void);
unsigned systick_elapsed(void);

#endif //__SYSTICK_H__
