// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

/**
 * @addtogroup SystemMain System: Main Program
 * @{
 */

#ifndef _PBSYS_MAIN_H_
#define _PBSYS_MAIN_H_

#include <pbsys/config.h>

#if PBSYS_CONFIG_MAIN

/** A main function. */
typedef void (*pbsys_main_t)(void);

__attribute__((noreturn)) void pbsys_main(pbsys_main_t main);

#endif // PBSYS_CONFIG_MAIN

#endif // _PBSYS_MAIN_H_

/** @} */
