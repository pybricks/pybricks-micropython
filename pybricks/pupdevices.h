// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/obj.h"

#include <pbio/iodev.h>
#include <pybricks/tools/pb_type_awaitable.h>

/**
 * Used in place of mp_obj_base_t in all pupdevices. This lets us share
 * code for awaitables for all pupdevices.
 */
typedef struct _pb_pupdevices_obj_base_t {
    mp_obj_base_t base;
    pbio_iodev_t *iodev;
    mp_obj_t awaitables;
} pb_pupdevices_obj_base_t;

/**
 * Callable sensor method with a particular mode and return map.
 */
typedef struct {
    mp_obj_base_t base;
    pb_type_awaitable_return_t get_values;
    uint8_t mode;
} pb_obj_pupdevices_method_t;

extern const mp_obj_type_t pb_type_pupdevices_method;

#define PB_DEFINE_CONST_PUPDEVICES_METHOD_OBJ(obj_name, mode_id, get_values_func) \
    const pb_obj_pupdevices_method_t obj_name = \
    {{&pb_type_pupdevices_method}, .mode = mode_id, .get_values = get_values_func}

extern const mp_obj_type_t pb_type_pupdevices_ColorDistanceSensor;
extern const mp_obj_type_t pb_type_pupdevices_ColorLightMatrix;
extern const mp_obj_type_t pb_type_pupdevices_ColorSensor;
extern const mp_obj_type_t pb_type_pupdevices_ForceSensor;
extern const mp_obj_type_t pb_type_pupdevices_InfraredSensor;
extern const mp_obj_type_t pb_type_pupdevices_Light;
extern const mp_obj_type_t pb_type_pupdevices_PFMotor;
extern const mp_obj_type_t pb_type_pupdevices_Remote;
extern const mp_obj_type_t pb_type_pupdevices_TiltSensor;
extern const mp_obj_type_t pb_type_pupdevices_UltrasonicSensor;

mp_obj_t pb_pupdevices_method_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);

pb_pupdevices_obj_base_t *pupdevices_ColorDistanceSensor__get_device(mp_obj_t obj);
void pb_type_Remote_cleanup(void);

void pb_pupdevices_init_class(pb_pupdevices_obj_base_t *self, mp_obj_t port_in, pbio_iodev_type_id_t valid_id);
mp_obj_t pb_pupdevices_set_data(pb_pupdevices_obj_base_t *sensor, uint8_t mode, const void *data);
void *pb_pupdevices_get_data(mp_obj_t self_in, uint8_t mode);
void *pb_pupdevices_get_data_blocking(pb_pupdevices_obj_base_t *sensor, uint8_t mode);

void pb_pup_device_setup_motor(pbio_port_id_t port, bool is_servo);

// Delete me: refactor getter only still used for passive external light.
pbio_iodev_t *pb_pup_device_get_device(pbio_port_id_t port, pbio_iodev_type_id_t valid_id);

#endif // PYBRICKS_PY_PUPDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
