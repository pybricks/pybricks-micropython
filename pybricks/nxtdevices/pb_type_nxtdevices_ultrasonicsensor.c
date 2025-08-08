// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/mphal.h"

#include <pbdrv/i2c.h>
#include <pbio/port_interface.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.UltrasonicSensor class object
typedef struct _nxtdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    pbdrv_i2c_dev_t *i2c_dev;
} nxtdevices_UltrasonicSensor_obj_t;

static void assert_i2c_string(pbdrv_i2c_dev_t *i2c_dev, const uint8_t address, const uint8_t reg, const char *string) {

    // REVISIT: Currently can't send back to back transactions. We need to
    // make pbdrv_i2c_write_then_read wait until it is ready to send. For now
    // harcode a delay.
    mp_hal_delay_ms(10);

    uint8_t tx[] = { reg };
    uint8_t rx[8] = { 0 };

    pbio_os_state_t state = 0;
    pbio_error_t err;

    while ((err = pbdrv_i2c_write_then_read(&state, i2c_dev, address, tx, sizeof(tx), rx, strlen(string), true)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK;
    }

    // Raise if not the expected string, so not the right device.
    if (memcmp(rx, string, strlen(string))) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }
}

const uint8_t i2c_address = 0x01;

// pybricks.nxtdevices.UltrasonicSensor.__init__
static mp_obj_t nxtdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pb_module_tools_assert_blocking();

    // Get the port instance and set to LEGO mode if it is not already set.
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_port_t *port;
    pb_assert(pbio_port_get_port(port_id, &port));
    pb_device_set_lego_mode(port);

    // Request access to I2C device and assert that expected device is attached.
    pbdrv_i2c_dev_t *i2c_dev;
    pb_assert(pbio_port_get_i2c_dev(port, &i2c_dev));
    assert_i2c_string(i2c_dev, i2c_address, 0x08, "LEGO");
    assert_i2c_string(i2c_dev, i2c_address, 0x10, "Sonar");


    nxtdevices_UltrasonicSensor_obj_t *self = mp_obj_malloc(nxtdevices_UltrasonicSensor_obj_t, type);
    self->i2c_dev = i2c_dev;
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.UltrasonicSensor.distance
static mp_obj_t nxtdevices_UltrasonicSensor_distance(mp_obj_t self_in) {
    // REVISIT: Currently can't send back to back transactions. We need to
    // make pbdrv_i2c_write_then_read wait until it is ready to send. For now
    // harcode a delay.
    mp_hal_delay_ms(10);

    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t tx[] = { 0x42 };
    uint8_t rx[1];

    pbio_os_state_t state = 0;
    pbio_error_t err;

    while ((err = pbdrv_i2c_write_then_read(&state, self->i2c_dev, i2c_address, tx, sizeof(tx), rx, sizeof(rx), true)) == PBIO_ERROR_AGAIN) {
        MICROPY_EVENT_POLL_HOOK;
    }
    pb_assert(err);

    // Scale cm to mm.
    return mp_obj_new_int(rx[0] * 10);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_UltrasonicSensor_distance_obj, nxtdevices_UltrasonicSensor_distance);

// dir(pybricks.nxtdevices.UltrasonicSensor)
static const mp_rom_map_elem_t nxtdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_distance_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_UltrasonicSensor_locals_dict, nxtdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.nxtdevices.UltrasonicSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_UltrasonicSensor,
    MP_QSTR_UltrasonicSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_UltrasonicSensor_make_new,
    locals_dict, &nxtdevices_UltrasonicSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
