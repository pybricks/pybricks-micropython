/* Driver for the NXT Embedded Flash Controller.
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/drivers/_efc.h"

void nx__efc_init(void) {
}

int nx__efc_write_page(U32 *data, int page) {
  U8 i;

  NX_ASSERT(page < EFC_PAGES);

  /* Write the page data to the flash in-memory mapping. */
  for (i=0 ; i<EFC_PAGE_WORDS ; i++) {
      FLASH_BASE[i+page*64] = data[i];

  /* Trigger the flash write command. */
      
}
