
#include <pbdrv/ioport.h>

#include "py/obj.h"

pbio_error_t pb_iodevice_get_mode(pbio_port_t port, uint8_t *current_mode);
pbio_error_t pb_iodevice_set_mode(pbio_port_t port, uint8_t new_mode);
mp_obj_t pb_iodevice_get_values(pbio_port_t port);
mp_obj_t pb_iodevice_set_values(pbio_port_t port, mp_obj_t values);
