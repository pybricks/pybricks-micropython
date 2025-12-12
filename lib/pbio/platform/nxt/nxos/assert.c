/* Copyright (C) 2007 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include <stdbool.h>
#include <string.h>

#include <pbdrv/reset.h>

#include "nxos/display.h"
#include "nxos/util.h"
#include "nxos/drivers/systick.h"

#include "nxos/assert.h"

void nx_assert_error(const char *file, const int line,
		     const char *expr, const char *msg) {
  const char *basename = strrchr(file, '/');
  basename = basename ? basename+1 : file;

  nx_display_clear();
  nx_display_string("** Assertion **\n");

  nx_display_string(basename);
  nx_display_string(":");
  nx_display_uint(line);
  nx_display_end_line();

  nx_display_string(expr);
  nx_display_end_line();

  nx_display_string(msg);
  nx_display_end_line();
}
