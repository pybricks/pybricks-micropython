/* Copyright (C) 2007 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include <stdbool.h>
#include <string.h>

#include <pbio/image.h>

#include <pbdrv/reset.h>
#include <pbdrv/display.h>

#include "nxos/util.h"
#include "nxos/drivers/systick.h"

#include "nxos/assert.h"

#include "../../../drv/display/display_nxt.h"

void nx_assert_error(const char *file, const int line,
		     const char *expr, const char *msg) {
  const char *basename = strrchr(file, '/');
  basename = basename ? basename+1 : file;

  pbio_image_t *display = pbdrv_display_get_image();
  pbio_image_fill(display, 0);
  pbio_image_print0(display, "** Assertion **\n");
  pbio_image_printf(display, "%s:%d\n", basename, line);
  pbio_image_printf(display, "%s\n", expr);
  pbio_image_printf(display, "%s\n", msg);
  pbdrv_display_nxt_sync_refresh();
}
