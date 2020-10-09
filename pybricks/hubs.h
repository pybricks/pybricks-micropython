// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_HUBS_H
#define PYBRICKS_INCLUDED_PYBRICKS_HUBS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS

#include "py/obj.h"

#if PYBRICKS_HUB_MOVEHUB
const mp_obj_type_t pb_type_MoveHub;
#endif
#if PYBRICKS_HUB_CITYHUB
const mp_obj_type_t pb_type_CityHub;
#endif
#if PYBRICKS_HUB_TECHNICHUB
const mp_obj_type_t pb_type_TechnicHub;
#endif
#ifdef PYBRICKS_HUB_PRIMEHUB
const mp_obj_type_t pb_type_PrimeHub;
#endif
#if PYBRICKS_HUB_NXTBRICK
const mp_obj_type_t pb_type_NXTBrick;
#endif
#if PYBRICKS_HUB_EV3BRICK
const mp_obj_type_t pb_type_EV3Brick;
#endif

const mp_obj_module_t pb_module_hubs;

#endif // PYBRICKS_PY_HUBS

#endif // PYBRICKS_INCLUDED_PYBRICKS_HUBS_H
