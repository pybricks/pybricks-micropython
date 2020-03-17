// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PYBRICKS_EXTMOD_PBERROR_H_
#define _PYBRICKS_EXTMOD_PBERROR_H_

#include <pbio/error.h>

#include "py/obj.h"

void pb_assert(pbio_error_t error);
void pb_assert_type(mp_obj_t obj, const mp_obj_type_t *type);

#endif // _PYBRICKS_EXTMOD_PBERROR_H_
