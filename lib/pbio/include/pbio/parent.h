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

/**
 * Callback function used by pbio_parent_set().
 *
 * Implementations should stop all actuation when this is called. For example,
 * when used with motors, this should stop the motors.
 *
 * @param [in]  parent          The parent object.
 * @param [in]  clear_parent    When true, the parent relationship is ending now.
 * @returns                     Error code on failure or ::PBIO_SUCCESS.
 */
typedef pbio_error_t (*pbio_parent_stop_func_t)(void *parent, bool clear_parent);

/** Opaque pointer for managing parent object relationships. */
typedef struct _pbio_parent_t {
    /** @private Pointer to the parent object or @c NULL if not currently linked. */
    void *parent_object;
    /** @private Stop function callback that is called from pbio_parent_stop().  */
    pbio_parent_stop_func_t parent_stop_func;
} pbio_parent_t;

/** @name Initialization Functions */
/**@{*/
void pbio_parent_set(pbio_parent_t *self, void *parent_object, pbio_parent_stop_func_t stop_func);
/**@}*/

/** @name Status Functions */
/**@{*/
bool pbio_parent_exists(const pbio_parent_t *self);
bool pbio_parent_equals(const pbio_parent_t *self, const void *candidate_object);
/**@}*/

/** @name Operation Functions */
/**@{*/
pbio_error_t pbio_parent_stop(pbio_parent_t *self, bool clear_parent);
/**@}*/

#endif // _PBIO_PARENT_H_

/** @} */
