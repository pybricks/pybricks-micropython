/* Copyright (C) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/assert.h"
#include "base/defrag.h"
#include "base/fs.h"
#include "base/_fs.h"
#include "base/util.h"

/* Move a @a len long flash region starting at page @a source to @a dest.
 * Since pages are moved one after another, regions may overlap if the
 * destination is lower in the flash than the source, but not the other
 * way around. Note that this is asserted anyway to avoid data loss.
 * It is the responsibility of the caller to clean up the remaining
 * source region of any data he doesn't want to leave there (file origin
 * markers for example).
 *
 * @param source The source page number.
 * @param dest The destination page number.
 * @param len The region length.
 */
static defrag_err_t nx_defrag_move_region(U32 source, U32 dest, U32 len) {
  U32 data;

  NX_ASSERT(source < EFC_PAGES);
  NX_ASSERT(dest < EFC_PAGES);
  NX_ASSERT(len < EFC_PAGES);
  NX_ASSERT(dest - source <= len);

  while (len--) {
    data = FLASH_BASE_PTR[source*EFC_PAGE_WORDS];
    if (!nx__efc_write_page(&data, dest)) {
      return DEFRAG_ERR_FLASH_ERROR;
    }

    source++;
    dest++;
  }

  return DEFRAG_ERR_NO_ERROR;
}

defrag_err_t nx_defrag_simple(void) {
  return DEFRAG_ERR_NO_ERROR;
}

defrag_err_t nx_defrag_for_file_by_name(char *name) {
  U32 origin;
  fs_err_t err;

  err = nx__fs_find_file_origin(name, &origin);
  if (err == FS_ERR_NO_ERROR) {
    return nx_defrag_for_file_by_origin(origin);
  }

  /* Fall back to simple defrag. */
  return nx_defrag_simple();
}

defrag_err_t nx_defrag_for_file_by_origin(U32 origin) {
  nx_defrag_move_region(origin, 1, 1); // TBR

  return DEFRAG_ERR_NO_ERROR;
}

defrag_err_t nx_defrag_best_overall(void) {
  return DEFRAG_ERR_NO_ERROR;
}
