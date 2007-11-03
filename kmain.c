
#include "at91sam7s256.h"

#include "base/types.h"
#include "base/interrupts.h"
#include "drivers/aic.h"
#include "drivers/systick.h"
#include "drivers/sound.h"
#include "drivers/avr.h"
#include "drivers/motors.h"
#include "drivers/lcd.h"
#include "base/_display.h"
#include "drivers/sensors.h"
#include "drivers/usb.h"

/* main() is the entry point into the custom payload, not included in
 * the NxOS core.
 */
extern void main();

static void core_init() {
  nx_aic_init();
  nx_interrupts_enable();
  nx_systick_init();
  nx_sound_init();
  nx_avr_init();
  nx_motors_init();
  nx_lcd_init();
  nx_display_init();
  nx_sensors_init();
  nx_usb_init();

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

static void core_shutdown() {
  nx_lcd_shutdown();
  nx_usb_disable();
  nx_avr_power_down();
}

void nx_kernel_main() {
  core_init();
  check_boot_errors();
  main();
  core_shutdown();
}
