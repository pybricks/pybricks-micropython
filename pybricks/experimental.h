// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_EXPERIMENTAL_H
#define PYBRICKS_INCLUDED_PYBRICKS_EXPERIMENTAL_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#include "py/obj.h"

#if PYBRICKS_PY_BLE

void pb_type_Broadcast_cleanup(void);
extern const mp_obj_type_t pb_type_Broadcast;

#else
static inline void pb_type_Broadcast_cleanup(void) {
}

#endif // PYBRICKS_PY_BLE

#endif // PYBRICKS_PY_EXPERIMENTAL

#endif // PYBRICKS_INCLUDED_PYBRICKS_EXPERIMENTAL_H
