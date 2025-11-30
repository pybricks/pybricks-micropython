#include <stdint.h>

#include "hardware/sync.h"

typedef uint32_t pbio_os_irq_flags_t;

static inline pbio_os_irq_flags_t pbio_os_hook_disable_irq(void) {
    return save_and_disable_interrupts();
}

static inline void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags) {
    restore_interrupts(flags);
}

static inline void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags) {
    __wfi();
}
