// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup SystemApp System: Application-specific config
 *
 * The following macros must be defined in a "pbsys_app_config.h" file by the
 * application that is using pbsys.
 *
 * @{
 */

#ifndef _PBSYS_APP_H_
#define _PBSYS_APP_H_


#include "pbsys_app_config.h"

#if DOXYGEN
/**
 * Specifies the hub features enabled by the application. See ::pbio_pybricks_feature_flags_t.
 */
#define PBSYS_APP_HUB_FEATURE_FLAGS
#endif
#ifndef PBSYS_APP_HUB_FEATURE_FLAGS
#error "Application must define PBSYS_APP_HUB_FEATURE_FLAGS"
#endif

#endif // _PBSYS_APP_H_

/** @} */
