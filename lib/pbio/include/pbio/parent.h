// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 LEGO System A/S

/**
 * @addtogroup Parent pbio/parent: Hierarchical motor resource management
 *
 * Safely manages motor resources used by multiple higher-level objects.
 * @{
 */

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

bool pbio_parent_exists(const pbio_parent_t *parent);

bool pbio_parent_equals(const pbio_parent_t *parent, const void *candidate_object);

pbio_error_t pbio_parent_stop(pbio_parent_t *self, bool clear_parent);

#endif // _PBIO_PARENT_H_

/** @} */
