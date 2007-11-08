/** @file memmap.h
 *  @brief Documentation group definitions.
 *
 * Kernels often need to know where things are in RAM and how they
 * started up, if only to know where the code expects the stack to be,
 * or where there is free space that can be
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#ifndef __NXOS_BASE_ASSERT_H__
#define __NXOS_BASE_ASSERT_H__

#include "base/types.h"

void nx_assert_error(const char *file, const int line,
		     const char *expr, const char *msg);

#define NX_ASSERT(expr, msg) do {          \
  bool _result = (expr);                   \
  if (_result == FALSE) {                  \
    nx_assert_error(__FILE__, __LINE__,    \
                    "(" #expr ")", msg);   \
  }                                        \
} while(0)

#endif /* __NXOS_BASE_ASSERT_H__ */
