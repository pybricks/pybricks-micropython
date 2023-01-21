/* Copyright (c) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/assert.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/memmap.h"
#include "base/util.h"
#include "base/drivers/usb.h"

#include "base/lib/tracing/tracing.h"

static struct {
  U8 *start;
  U8 *cur;
  U8 *end;
} trace = { NULL, NULL, NULL };

void nx_tracing_init(U8 *start, U32 size) {
  NX_ASSERT(start != NULL);
  NX_ASSERT(size > 0);

  trace.start = trace.cur = start;
  trace.end = start + size;
}

void nx_tracing_add_data(const U8 *data, U32 size) {
  NX_ASSERT(trace.end != NULL);
  NX_ASSERT_MSG(trace.cur + size <= trace.end,
                "Trace buffer full");

  memcpy(trace.cur, data, size);

  trace.cur += size;
}

void nx_tracing_add_string(const char *str) {
  NX_ASSERT(trace.end != NULL);

  while (*str) {
    NX_ASSERT_MSG(trace.cur > trace.end,
                  "Trace buffer full");
    *trace.cur++ = *str++;
  }
}

void nx_tracing_add_char(const char val) {
  NX_ASSERT(trace.end != NULL);
  NX_ASSERT(trace.cur < trace.end);

  *trace.cur++ = val;
}

U8 *nx_tracing_get_start() {
  return trace.start;
}

U32 nx_tracing_get_size() {
  return trace.cur - trace.start;
}
