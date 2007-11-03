
#include "at91sam7s256.h"

#include "mytypes.h"
#include "interrupts.h"
#include "drivers/aic.h"
#include "drivers/systick.h"
#include "drivers/sound.h"
#include "drivers/avr.h"
#include "drivers/motors.h"
#include "drivers/lcd.h"
#include "display.h"
#include "drivers/sensors.h"
#include "drivers/usb.h"

/* main() is the entry point into the custom payload, not included in
 * the NxOS core.
 */
extern void main();

static void core_init() {
  aic_init();
  interrupts_enable();
  systick_init();
  sound_init();
  avr_init();
  motors_init();
  lcd_init();
  display_init();
  sensors_init();
  usb_init();

  /* Delay a little post-init, to let all the drivers settle down. */
  systick_wait_ms(100);
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
    display_string("**************\n");
    display_string("Watchdog fault\n");
    display_string("**************\n");
    while (1);
  } else if (reset_status == AT91C_RSTC_RSTTYP_BROWNOUT) {
    display_string("**************\n");
    display_string("Brownout fault\n");
    display_string("**************\n");
    while (1);
  }
}

static void core_shutdown() {
  lcd_shutdown();
  usb_disable();
  avr_power_down();
}

void kernel_main() {
  core_init();
  check_boot_errors();
  main();
  core_shutdown();
}
