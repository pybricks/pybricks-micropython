// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup SystemLight System: Built-in Lights
 * @{
 */

#ifndef _PBSYS_LIGHT_H_
#define _PBSYS_LIGHT_H_

#include <pbio/light.h>
#include <pbsys/config.h>

#if PBSYS_CONFIG_STATUS_LIGHT
extern pbio_color_light_t *pbsys_status_light;
#endif

#endif // _PBSYS_LIGHT_H_

/** @} */
