#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include "extmod/utime_mphal.h"
#include "py/mperrno.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "modmotor.h"
#include "modhubcommon.h"
#include "pberror.h"
#include "pbobj.h"

#include "mpconfigbrick.h"

/* Movehub builtin motors */

#if PBIO_CONFIG_ENABLE_MOTORS
const mp_obj_type_t motor_MovehubMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_MovehubMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};
#endif //PBIO_CONFIG_ENABLE_MOTORS

/* Movehub ports */

STATIC const mp_rom_map_elem_t movehub_Port_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_A),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_A) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_B),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_B) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_C),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_C) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_D) },
};
STATIC PB_DEFINE_CONST_ENUM(movehub_Port_enum, movehub_Port_enum_table);

STATIC mp_obj_t hub_get_values(mp_obj_t port) {
    mp_obj_t values[PBIO_IODEV_MAX_DATA_SIZE];
    pbio_iodev_t *iodev;
    uint8_t *data;
    uint8_t len, i;
    pbio_iodev_data_type_t type;

    pb_assert(pbdrv_ioport_get_iodev(mp_obj_get_int(port), &iodev));
    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    pb_assert(pbio_iodev_get_bin_format(iodev, &len, &type));

    // this shouldn't happen, but just in case...
    if (len == 0) {
        return mp_const_none;
    }

    for (i = 0; i < len; i++) {
        switch (type) {
        case PBIO_IODEV_DATA_TYPE_INT8:
            values[i] = mp_obj_new_int(data[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_INT16:
            values[i] = mp_obj_new_int(*(int16_t *)(data + i * 2));
            break;
        case PBIO_IODEV_DATA_TYPE_INT32:
            values[i] = mp_obj_new_int(*(int32_t *)(data + i * 4));
            break;
        case PBIO_IODEV_DATA_TYPE_FLOAT:
            #if MICROPY_PY_BUILTINS_FLOAT
            values[i] = mp_obj_new_float(*(float *)(data + i * 4));
            #else // MICROPY_PY_BUILTINS_FLOAT
            // there aren't any known devices that use float data, so hopefully we will never hit this
            mp_raise_OSError(MP_EOPNOTSUPP);
            #endif // MICROPY_PY_BUILTINS_FLOAT
            break;
        default:
            mp_raise_NotImplementedError("Unknown data type");
        }
    }

    // if there are more than one value, pack them in a tuple
    if (len > 1) {
        return mp_obj_new_tuple(len, values);
    }

    // otherwise return the one value
    return values[0];
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_get_values_obj, hub_get_values);

STATIC mp_obj_t hub_set_values(mp_obj_t port, mp_obj_t values) {
    uint8_t data[PBIO_IODEV_MAX_DATA_SIZE];
    pbio_iodev_t *iodev;
    mp_obj_t *items;
    uint8_t len, i;
    pbio_iodev_data_type_t type;

    pb_assert(pbdrv_ioport_get_iodev(mp_obj_get_int(port), &iodev));
    pb_assert(pbio_iodev_get_bin_format(iodev, &len, &type));

    // if we only have one value, it doesn't have to be a tuple/list
    if (len == 1 && (mp_obj_is_integer(values)
        #if MICROPY_PY_BUILTINS_FLOAT
        || mp_obj_is_float(values)
        #endif
    )) {
        items = &values;
    }
    else {
        mp_obj_get_array_fixed_n(values, len, &items);
    }

    for (i = 0; i < len; i++) {
        switch (type) {
        case PBIO_IODEV_DATA_TYPE_INT8:
            data[i] = mp_obj_get_int(items[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_INT16:
            *(int16_t *)(data + i * 2) = mp_obj_get_int(items[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_INT32:
            *(int32_t *)(data + i * 4) = mp_obj_get_int(items[i]);
            break;
        case PBIO_IODEV_DATA_TYPE_FLOAT:
            #if MICROPY_PY_BUILTINS_FLOAT
            *(float *)(data + i * 4) = mp_obj_get_float(items[i]);
            #else // MICROPY_PY_BUILTINS_FLOAT
            // there aren't any known devices that use float data, so hopefully we will never hit this
            mp_raise_OSError(MP_EOPNOTSUPP);
            #endif // MICROPY_PY_BUILTINS_FLOAT
            break;
        default:
            mp_raise_NotImplementedError("Unknown data type");
        }
    }

    pb_assert(pbio_iodev_set_raw_values(iodev, data));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(hub_set_values_obj, hub_set_values);

STATIC mp_obj_t hub_set_mode(mp_obj_t port, mp_obj_t mode) {
    pbio_iodev_t *iodev;
    pb_assert(pbdrv_ioport_get_iodev(mp_obj_get_int(port), &iodev));
    pb_assert(pbio_iodev_set_mode(iodev, mp_obj_get_int(mode)));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(hub_set_mode_obj, hub_set_mode);

/* Movehub module table */

extern const struct _mp_obj_module_t pb_module_battery;

STATIC const mp_map_elem_t movehub_globals_table[] = {
    /* Unique to Movehub */
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_movehub) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Port), (mp_obj_t)&movehub_Port_enum },
#if PBIO_CONFIG_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_MovehubMotor), (mp_obj_t)&motor_MovehubMotor_type},
#endif //PBIO_CONFIG_ENABLE_MOTORS
    /* Common to Powered Up hubs */
    { MP_ROM_QSTR(MP_QSTR_wait), (mp_obj_t)&mp_utime_sleep_ms_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Color), (mp_obj_t)&pup_Color_enum },
    { MP_OBJ_NEW_QSTR(MP_QSTR_battery), (mp_obj_t)&pb_module_battery },
    { MP_OBJ_NEW_QSTR(MP_QSTR_shutdown), (mp_obj_t)&hub_shutdown_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reboot), (mp_obj_t)&hub_reboot_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_update), (mp_obj_t)&hub_update_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_light), (mp_obj_t)&hub_set_light_obj },
#if PYBRICKS_ENABLE_HARDWARE_DEBUG
    { MP_OBJ_NEW_QSTR(MP_QSTR_gpios), (mp_obj_t)&hub_gpios_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_adc), (mp_obj_t)&hub_read_adc_obj },
#endif //PYBRICKS_ENABLE_HARDWARE_DEBUG
    // hacks
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_values),  (mp_obj_t)&hub_get_values_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_values),  (mp_obj_t)&hub_set_values_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_mode),    (mp_obj_t)&hub_set_mode_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_movehub_globals,
    movehub_globals_table
);

const mp_obj_module_t pb_module_movehub = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_movehub_globals,
};
