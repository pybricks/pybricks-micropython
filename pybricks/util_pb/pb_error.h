// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PYBRICKS_EXTMOD_PB_ERROR_H_
#define _PYBRICKS_EXTMOD_PB_ERROR_H_

#include <pbio/error.h>

int pb_errcode_from_pbio_error(pbio_error_t error);
void pb_assert(pbio_error_t error);

#endif // _PYBRICKS_EXTMOD_PB_ERROR_H_
