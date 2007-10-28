#ifndef __NXOS_AIC_H__
#define __NXOS_AIC_H__

#include "mytypes.h"

typedef U32 aic_vector_t;

/* Priority levels for interrupt lines. */
typedef enum {
  AIC_PRIO_LOW = 1,     /* User and soft real time tasks. */
  AIC_PRIO_DRIVER = 3,  /* Most drivers go in here. */
  AIC_PRIO_SOFTMAC = 5, /* Drivers that have no hardware controller. */
  AIC_PRIO_RT = 6,      /* Hard real time tasks that musn't displace
                         * timekeeping. */
  AIC_PRIO_TICK = 7,    /* Hard real time tasks (system time, AVR link). */
} aic_priority_t;

typedef enum {
  AIC_TRIG_LEVEL = 0,   /* Level-triggered interrupt. */
  AIC_TRIG_EDGE = 1,    /* Edge-triggered interrupt. */
} aic_trigger_mode_t;

typedef void (*aic_isr_t)();

void aic_init();
void aic_install_isr(aic_vector_t vector, aic_priority_t prio,
                     aic_trigger_mode_t trig_mode, aic_isr_t isr);
void aic_enable(aic_vector_t vector);
void aic_disable(aic_vector_t vector);
void aic_set(aic_vector_t vector);
void aic_clear(aic_vector_t vector);

#endif
