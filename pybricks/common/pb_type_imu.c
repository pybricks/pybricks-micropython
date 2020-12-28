// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU

#include "py/obj.h"

#include <lsm6ds3tr_c_reg.h>

#include <pybricks/common.h>

#include <pybricks/util_pb/pb_imu.h>

typedef struct _common_IMU_obj_t {
    mp_obj_base_t base;
    pb_imu_dev_t *imu_dev;
} common_IMU_obj_t;

STATIC mp_obj_t common_IMU_acceleration(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int16_t data[3];
    pb_imu_accel_read(self->imu_dev, data);

    mp_obj_t values[3];
    values[0] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs2g_to_mg(data[0]) / 1000.0f);
    values[1] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs2g_to_mg(data[1]) / 1000.0f);
    values[2] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs2g_to_mg(data[2]) / 1000.0f);

    return mp_obj_new_tuple(3, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_acceleration_obj, common_IMU_acceleration);

STATIC mp_obj_t common_IMU_gyro(mp_obj_t self_in) {
    common_IMU_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int16_t data[3];
    pb_imu_gyro_read(self->imu_dev, data);

    mp_obj_t values[3];
    values[0] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs250dps_to_mdps(data[0]) / 1000.0f);
    values[1] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs250dps_to_mdps(data[1]) / 1000.0f);
    values[2] = mp_obj_new_float_from_f(lsm6ds3tr_c_from_fs250dps_to_mdps(data[2]) / 1000.0f);

    return mp_obj_new_tuple(3, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_IMU_gyro_obj, common_IMU_gyro);

// dir(pybricks.common.IMU)
STATIC const mp_rom_map_elem_t common_IMU_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_accel), MP_ROM_PTR(&common_IMU_acceleration_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyro),  MP_ROM_PTR(&common_IMU_gyro_obj)  },
};
STATIC MP_DEFINE_CONST_DICT(common_IMU_locals_dict, common_IMU_locals_dict_table);

// type(pybricks.common.IMU)
STATIC const mp_obj_type_t pb_type_IMU = {
    { &mp_type_type },
    .name = MP_QSTR_IMU,
    .locals_dict = (mp_obj_dict_t *)&common_IMU_locals_dict,
};

STATIC common_IMU_obj_t singleton_obj;

// pybricks._common.IMU.__init__
mp_obj_t pb_type_IMU_obj_new(void) {

    // Get singleton instance
    common_IMU_obj_t *self = &singleton_obj;

    // Return if already initialized
    if (self->imu_dev) {
        return self;
    }

    self->base.type = &pb_type_IMU;

    pb_imu_get_imu(&self->imu_dev);

    // Initialize IMU
    pb_imu_init(self->imu_dev);

    return self;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_IMU
