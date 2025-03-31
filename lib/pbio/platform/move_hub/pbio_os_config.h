#include <stdint.h>

#include "stm32f0xx.h"

typedef uint32_t pbio_os_irq_flags_t;

static inline pbio_os_irq_flags_t pbio_os_hook_disable_irq(void) {
    uint32_t flags = __get_PRIMASK();
    __disable_irq();
    return flags;
}

static inline void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags) {
    __set_PRIMASK(flags);
}

static inline void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags) {
    __WFI();
}
