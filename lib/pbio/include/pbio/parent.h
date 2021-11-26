// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_PARENT_H_
#define _PBIO_PARENT_H_

#include <stdbool.h>

#include <pbio/error.h>


typedef pbio_error_t (*pbio_parent_stop_func_t)(void *parent, bool clear_parent);

typedef struct _pbio_parent_t {
    void *parent_object;
    pbio_parent_stop_func_t parent_stop_func;
} pbio_parent_t;

void pbio_parent_set(pbio_parent_t *parent, void *parent_object, pbio_parent_stop_func_t stop_func);

pbio_error_t pbio_parent_stop(pbio_parent_t *self, bool clear_parent);

#endif // _PBIO_PARENT_H_
