#ifndef __NXOS_AVR_H__
#define __NXOS_AVR_H__

#include "mytypes.h"

typedef enum {
  BUTTON_NONE = 0,
  BUTTON_OK,
  BUTTON_CANCEL,
  BUTTON_LEFT,
  BUTTON_RIGHT,
} avr_button_t;

void avr_init();
void avr_fast_update();

void avr_power_down();
void avr_firmware_update_mode();

avr_button_t avr_get_button();
U32 avr_get_battery_voltage();
bool avr_battery_is_aa();
U32 avr_get_sensor_value(U32 sensor);
void avr_get_version(U8 *major, U8 *minor);

void avr_set_motor(U32 motor, int power_percent, bool brake);
void avr_set_input_power(U32 sensor, U32 power_type);

#endif
