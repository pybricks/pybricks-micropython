// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/obj.h"

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

pbio_iodev_t *pupdevices_ColorDistanceSensor__get_device(mp_obj_t obj);
void pb_type_Remote_cleanup(void);

pbio_iodev_t *pb_pup_device_get_device(pbio_port_id_t port, pbio_iodev_type_id_t valid_id);
void *pb_pup_device_get_data(pbio_iodev_t *iodev, uint8_t mode);
void pb_pup_device_set_data(pbio_iodev_t *iodev, uint8_t mode, const void *data);

void pb_pup_device_setup_motor(pbio_port_id_t port, bool is_servo);

#endif // PYBRICKS_PY_PUPDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
