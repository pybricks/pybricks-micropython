// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

/**
 * @addtogroup SystemMain System: Main Program
 * @{
 */

#ifndef _PBSYS_MAIN_H_
#define _PBSYS_MAIN_H_

/** A main function. */
typedef void (*pbsys_main_t)(void);

__attribute__((noreturn)) void pbsys_main(pbsys_main_t main);

#endif // _PBSYS_MAIN_H_

/** @} */
