#include <stdint.h>

typedef uint32_t pbio_os_irq_flags_t;

static inline pbio_os_irq_flags_t pbio_os_hook_disable_irq(void) {
    return 0;
}

static inline void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags) {
}

static inline void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags) {
    // All events have been handled at this time. Advance the clock
    // and continue immediately.
    extern void pbio_test_clock_tick(uint32_t ticks);
    pbio_test_clock_tick(1);
}
