#include <stdio.h>

#include <pbdrv/battery.h>
#include <pbio/light.h>
#include <pbsys/sys.h>

#include "py/obj.h"

#include "mpconfigbrick.h"
#include "modmotor.h"
#include "modhubcommon.h"
#include "pberror.h"
#include "pbobj.h"


STATIC mp_obj_t hub_shutdown(void) {
    pbsys_power_off();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hub_shutdown_obj, hub_shutdown);

STATIC mp_obj_t hub_reboot() {
    pbsys_reboot(0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hub_reboot_obj, hub_reboot);

STATIC mp_obj_t hub_update() {
    pbsys_reboot(1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(hub_update_obj, hub_update);

STATIC mp_obj_t hub_set_light(mp_obj_t color) {
    // TODO: None argument should turn light off
    pbio_light_color_t color_id = mp_obj_get_int(color);
    if (color_id < PBIO_LIGHT_COLOR_NONE || color_id > PBIO_LIGHT_COLOR_PURPLE) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    if (color_id == PBIO_LIGHT_COLOR_NONE) {
        pbio_light_off(PBIO_PORT_SELF);
    }
    else {
        pbio_light_on(PBIO_PORT_SELF, color_id);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_set_light_obj, hub_set_light);
