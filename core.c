
#include "at91sam7s256.h"

#include "base/types.h"
#include "base/interrupts.h"
#include "drivers/_aic.h"
#include "drivers/_systick.h"
#include "drivers/_sound.h"
#include "drivers/_avr.h"
#include "drivers/_motors.h"
#include "drivers/_lcd.h"
#include "base/_display.h"
#include "drivers/_sensors.h"
#include "drivers/_usb.h"

/* main() is the entry point into the custom payload, not included in
 * the NxOS core.
 */
extern void main();

static void core_init() {
  nx__aic_init();
  nx_interrupts_enable();
  nx__systick_init();
  nx__sound_init();
  nx__avr_init();
  nx__motors_init();
  nx__lcd_init();
  nx__display_init();
  nx__sensors_init();
  nx__usb_init();

  /* Delay a little post-init, to let all the drivers settle down. */
  nx_systick_wait_ms(100);
}

/* Checks whether the system rebooted due to some kind of external
 * failure, and report if so.
 *
 * Currently detects brownout and watchdog resets.
 */
static void check_boot_errors() {
  int reset_status = *AT91C_RSTC_RSR;
  reset_status &= AT91C_RSTC_RSTTYP;

  /* A watchdog reset should never happen, given that we disable it
   * immediately at bootup. This is just a bug guard.
   */
  if (reset_status == AT91C_RSTC_RSTTYP_WATCHDOG) {
    nx_display_string("**************\n");
    nx_display_string("Watchdog fault\n");
    nx_display_string("**************\n");
    while (1);
  } else if (reset_status == AT91C_RSTC_RSTTYP_BROWNOUT) {
    nx_display_string("**************\n");
    nx_display_string("Brownout fault\n");
    nx_display_string("**************\n");
    while (1);
  }
}

void nx_core_halt() {
  nx__lcd_shutdown();
  nx__usb_disable();
  nx__avr_power_down();
}

/* This function is not part of the public API, but is invoked from
 * init.S.
 */
void nx_kernel_main() {
  core_init();
  check_boot_errors();
  main();
  nx_core_halt();
}
