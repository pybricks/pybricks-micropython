#include <stdint.h>

#include <tiam1808/armv5/am1808/interrupt.h>

typedef uint32_t pbio_os_irq_flags_t;

static inline pbio_os_irq_flags_t pbio_os_hook_disable_irq(void) {
    return IntDisable();
}

static inline void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags) {
    IntEnable(flags);
}

static inline void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags) {
    __asm volatile (
        "mov	r0, #0\n"
        "mcr	p15, 0, r0, c7, c0, 4\n"
        ::: "r0"
        );
}
