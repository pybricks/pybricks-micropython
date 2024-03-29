/* Copyright (c) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

/* Driver for the NXT Embedded Flash Controller.
 */

#include <stdbool.h>
#include <stdint.h>

#include <at91sam7s256.h>

#include "nxos/nxt.h"
#include "nxos/interrupts.h"
#include "nxos/assert.h"
#include "nxos/drivers/systick.h"
#include "nxos/drivers/_efc.h"

#define EFC_WRITE ((EFC_WRITE_KEY << 24) + EFC_CMD_WP)
#define EFC_THROTTLE_TIMER 2

void nx__efc_init(void) {
}

static bool nx__efc_do_write(uint32_t page) {
  uint32_t ret;

  NX_ASSERT(page < EFC_PAGES);

  /* Trigger the flash write command. */
  *AT91C_MC_FCR = EFC_WRITE + ((page & 0x000003FF) << 8);

  /* Wait for the command to complete. */
  do {
    ret = *AT91C_MC_FSR;
  } while (!(ret & AT91C_MC_FRDY));

  /* Check the command result by reading the status register
   * only once to avoid the bits being cleared.
   */
  if (ret & AT91C_MC_LOCKE || ret & AT91C_MC_PROGE)
    return false;

  return true;
}

static inline void nx__efc_wait_for_flash(void) {
  while (!(*AT91C_MC_FSR & AT91C_MC_FRDY));
}

/* Write one page at the given page number in the flash.
 * Use interrupt-driven flash programming ?
 */
bool nx__efc_write_page(uint32_t *data, uint32_t page) {
  uint8_t i;

  NX_ASSERT(page < EFC_PAGES);

  /* Wait for the flash to be ready. */
  nx__efc_wait_for_flash();
  nx_systick_wait_ms(EFC_THROTTLE_TIMER);

  /* Write the page data to the flash in-memory mapping. */
  for (i=0 ; i<EFC_PAGE_WORDS ; i++) {
      FLASH_BASE_PTR[i+page*EFC_PAGE_WORDS] = data[i];
  }

  return nx__efc_do_write(page);
}

/* TODO: figure out if its faster or not to retrieve the
 * data or just a pointer, relative to the data retrieval
 * mechanism from the flash.
 */
void nx__efc_read_page(uint32_t page, uint32_t *data) {
  uint8_t i;

  NX_ASSERT(page < EFC_PAGES);

  nx_systick_wait_ms(EFC_THROTTLE_TIMER);

  for (i=0; i<EFC_PAGE_WORDS; i++) {
    data[i] = FLASH_BASE_PTR[page*EFC_PAGE_WORDS+i];
  }
}

bool nx__efc_erase_page(uint32_t page, uint32_t value) {
  uint8_t i;

  NX_ASSERT(page < EFC_PAGES);

  /* Wait for the flash to be ready. */
  nx__efc_wait_for_flash();
  nx_systick_wait_ms(EFC_THROTTLE_TIMER);

  /* Write the page data to the flash in-memory mapping. */
  for (i=0 ; i<EFC_PAGE_WORDS ; i++) {
      FLASH_BASE_PTR[i+page*EFC_PAGE_WORDS] = value;
  }

  return nx__efc_do_write(page);
}

/* TODO: implement other flash operations? */

