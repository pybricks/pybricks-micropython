#include <signal.h>

typedef sigset_t pbio_os_irq_flags_t;

pbio_os_irq_flags_t pbio_os_hook_disable_irq(void);

void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags);

void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags);
