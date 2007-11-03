#include "base/at91sam7s256.h"
#include "base/mytypes.h"
#include "base/interrupts.h"
#include "base/drivers/aic.h"
#include "base/drivers/avr.h"
#include "base/drivers/lcd.h"
#include "base/drivers/systick.h"


/* The main clock is at 48MHz, and the PIT divides that by 16 to get
 * its base timer frequency.
 */
#define PIT_BASE_FREQUENCY (48000000/16)

/* We want a timer interrupt 1000 times per second. */
#define SYSIRQ_FREQ 1000

/* The system IRQ processing takes place in two different interrupt
 * handlers: the main PIT interrupt handler runs at a high priority,
 * keeps the system time accurate, and triggers the lower priority
 * IRQ.
 *
 * All other periodic processing (needed eg. by drivers) takes place
 * in this lower priority interrupt handler. This ensures that we
 * don't miss a clock beat.
 *
 * As the PIT only has one interrupt line, we steal the PWM
 * controller's interrupt line and use it as our low-priority system
 * interrupt. The PWM controller is unused on the NXT, so this is no
 * problem.
 */
#define SCHEDULER_SYSIRQ AT91C_ID_PWMC


/* The system timer. Counts the number of milliseconds elapsed since
 * the system's initialization.
 */
static volatile U32 systick_time;

/* The scheduler callback. Application kernels can set this to their own
 * callback function, to do scheduling in the high priority systick
 * interrupt.
 */
static nx_closure_t scheduler_cb = NULL;

/* Low priority handler, called 1000 times a second by the high
 * priority handler.
 */
static void systick_sched() {
  /* Acknowledge the interrupt. */
  nx_aic_clear(SCHEDULER_SYSIRQ);

  /* Call into the scheduler. */
  if (scheduler_cb)
    scheduler_cb();
}


/* High priority handler, called 1000 times a second */
static void systick_isr() {
  U32 status;
  /* The PIT's value register must be read to acknowledge the
   * interrupt.
   */
  status = *AT91C_PITC_PIVR;

  /* Do the system timekeeping. */
  systick_time++;

  /* Keeping up with the AVR link is a crucial task in the system, and
   * must absolutely be kept up with at all costs. Thus, handling it
   * in the low-level dispatcher is not enough, and we promote it to
   * being handled directly here.
   *
   * As a result, this handler must be *very* fast.
   */
  nx_avr_fast_update();

  /* The LCD dirty display routine can be done here too, since it is
   * very short.
   */
  nx_lcd_fast_update();

  /* If the application kernel set a scheduling callback, trigger the
   * lower priority IRQ in which the scheduler runs.
   */
  if (scheduler_cb)
    nx_aic_set(SCHEDULER_SYSIRQ);
}


/* Return the absolute system time. */
U32 nx_systick_get_ms() {
  return systick_time;
}


/* Enter a busy wait loop for the given number of milliseconds. */
void nx_systick_wait_ms(U32 ms) {
  U32 final = systick_time + ms;

  while (systick_time < final);
}


/* Enter a busy wait loop for the given number of nanoseconds. This
 * routine relies entirely on instruction timing within the CPU, and
 * is calibrated for 48MHz.
 */
void nx_systick_wait_ns(U32 ns) {
  volatile U32 x = (ns >> 7) + 1;

  while (x--);
}


/* Initialize the system timer facility. */
void nx_systick_init() {
  nx_interrupts_disable();

  /* Install both the low and high priority interrupt handlers, ready
   * to handle periodic updates.
   */
  nx_aic_install_isr(SCHEDULER_SYSIRQ, AIC_PRIO_SCHED,
		     AIC_TRIG_EDGE, systick_sched);
  nx_aic_install_isr(AT91C_ID_SYS, AIC_PRIO_TICK,
		     AIC_TRIG_EDGE, systick_isr);

  /* Configure and enable the Periodic Interval Timer. The counter
   * value is 1/16th of the master clock (base frequency), divided by
   * our desired interrupt period of 1ms.
   */
  *AT91C_PITC_PIMR = (((PIT_BASE_FREQUENCY / SYSIRQ_FREQ) - 1) |
                      AT91C_PITC_PITEN | AT91C_PITC_PITIEN);

  nx_interrupts_enable();
}

void nx_systick_install_scheduler(nx_closure_t sched_cb) {
  nx_interrupts_disable();
  scheduler_cb = sched_cb;
  nx_interrupts_enable();
}
