// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_MEDIA_H
#define PYBRICKS_INCLUDED_PYBRICKS_MEDIA_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_MEDIA

#include "py/obj.h"

#if PYBRICKS_PY_MEDIA_IMAGE

extern const mp_obj_type_t pb_type_Image;

#endif // PYBRICKS_PY_MEDIA_IMAGE

#endif // PYBRICKS_PY_MEDIA

#endif // PYBRICKS_INCLUDED_PYBRICKS_MEDIA_H
