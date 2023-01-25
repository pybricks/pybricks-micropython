// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2007 the NxOS developers
// See AUTHORS for a full list of the developers.
// Copyright (c) 2023 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_NXT

#include <stdint.h>

#include <at91sam7s256.h>
#include <contiki.h>
#include <nxos/nxt.h>
#include <nxos/interrupts.h>
#include <nxos/drivers/aic.h>
#include <nxos/drivers/_avr.h>
#include <nxos/drivers/_lcd.h>

/* The main clock is at 48MHz, and the PIT divides that by 16 to get
 * its base timer frequency.
 */
#define PIT_BASE_FREQUENCY (NXT_CLOCK_FREQ / 16)

/* We want a timer interrupt 1000 times per second. */
#define SYSIRQ_FREQ 1000

/* The system timer. Counts the number of milliseconds elapsed since
 * the system's initialization.
 */
static volatile uint32_t pbdrv_clock_ticks;

/* High priority handler, called 1000 times a second */
static void systick_isr(void) {
    /* The PIT's value register must be read to acknowledge the
     * interrupt.
     */
    uint32_t status = *AT91C_PITC_PIVR;
    (void)status;

    /* Do the system timekeeping. */
    pbdrv_clock_ticks++;
    etimer_request_poll();

    /* Keeping up with the AVR link is a crucial task in the system, and
     * must absolutely be kept up with at all costs.
     *
     * As a result, this handler must be *very* fast.
     */
    nx__avr_fast_update();

    /* The LCD dirty display routine can be done here too, since it is
     * very short.
     */
    nx__lcd_fast_update();
}

void pbdrv_clock_init(void) {
    uint32_t state = nx_interrupts_disable();

    nx_aic_install_isr(AT91C_ID_SYS, AIC_PRIO_TICK, AIC_TRIG_EDGE, systick_isr);

    /* Configure and enable the Periodic Interval Timer. The counter
     * value is 1/16th of the master clock (base frequency), divided by
     * our desired interrupt period of 1ms.
     */
    *AT91C_PITC_PIMR = (((PIT_BASE_FREQUENCY / SYSIRQ_FREQ) - 1) |
        AT91C_PITC_PITEN | AT91C_PITC_PITIEN);

    nx_interrupts_enable(state);
}

uint32_t pbdrv_clock_get_ms(void) {
    return pbdrv_clock_ticks;
}

uint32_t pbdrv_clock_get_100us(void) {
    // Revisit: derive from ns counter properly.
    return pbdrv_clock_ticks * 10;
}

uint32_t pbdrv_clock_get_us(void) {
    // TODO
    return pbdrv_clock_ticks * 1000;
}

// TODO: we really should get rid of blocking waits if possible

void nx_systick_wait_ms(uint32_t ms) {
    // FIXME: this does not currently handle overflow (pbdrv_clock_ticks + ms > UINT32_MAX)
    uint32_t final = pbdrv_clock_ticks + ms;

    while (pbdrv_clock_ticks < final) {
        ;
    }
}

void nx_systick_wait_ns(uint32_t ns) {
    volatile uint32_t x = (ns >> 7) + 1;

    while (x--) {
        ;
    }
}

#endif // PBDRV_CONFIG_CLOCK_NXT
