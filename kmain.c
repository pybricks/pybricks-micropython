
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

static void core_shutdown() {
  lcd_shutdown();
  usb_disable();
  avr_power_down();
}

void kernel_main() {
  core_init();
  main();
  core_shutdown();
}
