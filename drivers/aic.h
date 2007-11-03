#ifndef __NXOS_AIC_H__
#define __NXOS_AIC_H__

#include "base/types.h"

typedef U32 aic_vector_t;

/* Priority levels for interrupt lines. */
typedef enum {
  AIC_PRIO_LOW = 1,     /* User and soft real time tasks. */
  AIC_PRIO_DRIVER = 3,  /* Most drivers go in here. */
  AIC_PRIO_SOFTMAC = 4, /* Drivers that have no hardware controller. */
  AIC_PRIO_SCHED = 5,   /* The scheduler. It mustn't displace
			 * maintaining the AVR link, but can preempt
			 * everything else.
			 */
  AIC_PRIO_RT = 6,      /* Hard real time tasks that musn't displace
                         * timekeeping. */
  AIC_PRIO_TICK = 7,    /* Hard real time tasks (system time, AVR link). */
} aic_priority_t;

typedef enum {
  AIC_TRIG_LEVEL = 0,   /* Level-triggered interrupt. */
  AIC_TRIG_EDGE = 1,    /* Edge-triggered interrupt. */
} aic_trigger_mode_t;

typedef void (*aic_isr_t)();

void nx_aic_init();
void nx_aic_install_isr(aic_vector_t vector, aic_priority_t prio,
                     aic_trigger_mode_t trig_mode, aic_isr_t isr);
void nx_aic_enable(aic_vector_t vector);
void nx_aic_disable(aic_vector_t vector);
void nx_aic_set(aic_vector_t vector);
void nx_aic_clear(aic_vector_t vector);

#endif
