/* Driver for the NXT Embedded Flash Controller.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/assert.h"
#include "base/drivers/_efc.h"

void nx__efc_init(void) {
}

bool nx__efc_write_page(U32 *data, U32 page) {
  U8 i;

  NX_ASSERT(page < EFC_PAGES);

  /* Write the page data to the flash in-memory mapping. */
  for (i=0 ; i<EFC_PAGE_WORDS ; i++) {
      FLASH_BASE_PTR[i+page*EFC_PAGE_WORDS] = data[i];
  }

  /* Trigger the flash write command. */

  return TRUE;
}
