/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/types.h"
#include "base/display.h"
#include "base/core.h"
#include "base/util.h"
#include "base/drivers/sound.h"
#include "base/drivers/avr.h"

#include "base/assert.h"

void nx_assert_error(const char *file, const int line,
		     const char *expr, const char *msg) {
  const char *basename = strrchr(file, '/');
  basename = basename ? basename+1 : file;

  nx_display_clear();
  nx_sound_freq_async(440, 1000);
  nx_display_string("** Assertion **\n");

  nx_display_string(basename);
  nx_display_string(":");
  nx_display_uint(line);
  nx_display_end_line();

  nx_display_string(expr);
  nx_display_end_line();

  nx_display_string(msg);
  nx_display_end_line();

  while (nx_avr_get_button() != BUTTON_CANCEL);
  nx_core_halt();
}
