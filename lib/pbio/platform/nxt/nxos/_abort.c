/* Copyright (C) 2007 the NxOS developers
 *
 * See lib/pbio/platform/nxt/nxos/AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include <stdbool.h>
#include <stdint.h>

#include "nxos/display.h"
#include "nxos/interrupts.h"
#include "nxos/drivers/_lcd.h"

#include "nxos/_abort.h"

void nx__abort(bool data, uint32_t pc, uint32_t cpsr) {
  nx_interrupts_disable();
  nx_display_auto_refresh(false);
  nx_display_clear();
  if (data) {
    nx_display_string("Data");
  } else {
    nx_display_string("Prefetch");
  }
  nx_display_string(" abort\nPC: ");
  nx_display_hex(pc);
  nx_display_string("\nCPSR: ");
  nx_display_hex(cpsr);
  nx__lcd_sync_refresh();
  while(1);
}
