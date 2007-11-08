/** @file aic.h
 *  @brief Interrupt controller interface.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_DRIVERS_AIC_H__
#define __NXOS_BASE_DRIVERS_AIC_H__

#include "base/types.h"

/** @addtogroup driver */
/*@{*/

/** @defgroup aic Interrupt controller
 *
 * The interrupt controller driver allows registration of interrupt
 * handlers for hardware peripherals.
 */
/*@{*/

/** The vector number type. */
typedef U32 nx_aic_vector_t;

/** Priority levels for interrupt lines. */
typedef enum {
  AIC_PRIO_LOW = 1,     /**< User and soft real time tasks. */
  AIC_PRIO_DRIVER = 3,  /**< Most drivers go in here. */
  AIC_PRIO_SOFTMAC = 4, /**< Drivers that have no hardware controller. */
  AIC_PRIO_SCHED = 5,   /**< The scheduler.
                         *
                         * It doesn't displace maintaining the AVR
			 * link, but can preempt everything else.
			 */
  AIC_PRIO_RT = 6,      /**< Hard real time tasks.
                         *
                         * This is basically the TWI driver.
                         */
  AIC_PRIO_TICK = 7,    /* Hard real time tasks (system time, AVR link). */
} nx_aic_priority_t;

/** Interrupt trigger modes.
 *
 * The trigger mode to use usually depends on the specific hardware
 * peripheral being controlled. Consult the board documentation if you
 * need to set interrupt handlers.
 */
typedef enum {
  AIC_TRIG_LEVEL = 0,   /**< Level-triggered interrupt. */
  AIC_TRIG_EDGE = 1,    /**< Edge-triggered interrupt. */
} nx_aic_trigger_mode_t;

/** Install @a isr as the handler for @a vector.
 *
 * @param vector The interrupt vector to configure.
 * @param prio The interrupt's priority.
 * @param trig_mode The interrupt's trigger mode.
 * @param isr The routine to call when the interrupt occurs.
 *
 * @note The interrupt line @a vector is enabled once the handler is
 * installed. There is no need to call nx_aic_enable() yourself.
 *
 * @warning If you install an ISR for a peripheral that already has
 * one installed, you @b will replace the original handler. Use with
 * care!
 */
void nx_aic_install_isr(nx_aic_vector_t vector, nx_aic_priority_t prio,
                        nx_aic_trigger_mode_t trig_mode, nx_closure_t isr);

/** Enable dispatching of @a vector.
 *
 * @param vector The interrupt vector to enable.
 */
void nx_aic_enable(nx_aic_vector_t vector);

/** Disable dispatching of @a vector.
 *
 * @param vector The interrupt vector to disable.
 */
void nx_aic_disable(nx_aic_vector_t vector);

/** Manually trigger the interrupt line @a vector.
 *
 * @param vector The interrupt vector to trigger.
 */
void nx_aic_set(nx_aic_vector_t vector);

/** Manually reset the interrupt line @a vector.
 *
 * @param vector The interrupt vector to reset.
 *
 * @note This should only be needed in the cases where the interrupt
 * line was triggered manually with nx_aic_set(). In other cases, each
 * peripheral has its own discipline for acknowledging the interrupt.
 */
void nx_aic_clear(nx_aic_vector_t vector);

/*@}*/
/*@}*/

#endif
