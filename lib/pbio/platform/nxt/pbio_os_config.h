#include <stdint.h>

#include <nxos/interrupts.h>
#include <at91sam7s256.h>

typedef uint32_t pbio_os_irq_flags_t;

static inline pbio_os_irq_flags_t pbio_os_hook_disable_irq(void) {
    return nx_interrupts_disable();
}

static inline void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags) {
    nx_interrupts_enable(flags);
}

static inline void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags) {
    // disable the processor clock which puts it in Idle Mode.
    AT91C_BASE_PMC->PMC_SCDR = AT91C_PMC_PCK;
}
