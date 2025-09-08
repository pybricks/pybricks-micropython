// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup SystemLight System: Built-in Lights
 * @{
 */

#ifndef _PBSYS_LIGHT_H_
#define _PBSYS_LIGHT_H_

#include <pbsys/config.h>

#if PBSYS_CONFIG_STATUS_LIGHT
#include <pbio/light.h>
extern pbio_color_light_t *pbsys_status_light_main;
#endif

#if PBSYS_CONFIG_HUB_LIGHT_MATRIX

#include <pbio/light_matrix.h>

extern pbio_light_matrix_t *pbsys_hub_light_matrix;

void pbsys_hub_light_matrix_free_display(void);

#else // PBSYS_CONFIG_HUB_LIGHT_MATRIX

static inline void pbsys_hub_light_matrix_free_display(void) {
}

#endif

#endif // _PBSYS_LIGHT_H_

/** @} */
