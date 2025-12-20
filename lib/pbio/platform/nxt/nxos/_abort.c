/* Copyright (C) 2007 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include <stdbool.h>
#include <stdint.h>

#include <pbio/image.h>

#include <pbdrv/display.h>

#include "nxos/interrupts.h"

#include "nxos/_abort.h"

#include "../../../drv/display/display_nxt.h"

void nx__abort(bool data, uint32_t pc, uint32_t cpsr) {
  nx_interrupts_disable();
  pbio_image_t *display = pbdrv_display_get_image();
  pbio_image_fill(display, 0);
  if (data) {
    pbio_image_print0(display, "Data");
  } else {
    pbio_image_print0(display, "Prefetch");
  }
  pbio_image_print0(display, " abort\nPC: ");
  pbio_image_print_hex(display, pc);
  pbio_image_print0(display, "\nCPSR: ");
  pbio_image_print_hex(display, cpsr);
  pbdrv_display_nxt_sync_refresh();
  while(1);
}
