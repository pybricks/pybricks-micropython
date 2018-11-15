
#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/port.h>

void _pbdrv_button_init(void) {
    // TODO
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) { }
#endif

pbio_error_t pbdrv_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed) {
    // TODO
    return PBIO_SUCCESS;
}
