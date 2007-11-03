#ifndef __NXOS_AVR_H__
#define __NXOS_AVR_H__

#include "base/mytypes.h"

typedef enum {
  BUTTON_NONE = 0,
  BUTTON_OK,
  BUTTON_CANCEL,
  BUTTON_LEFT,
  BUTTON_RIGHT,
} avr_button_t;

void nx_avr_init();
void nx_avr_fast_update();

void nx_avr_power_down();
void nx_avr_firmware_update_mode();

avr_button_t nx_avr_get_button();
U32 nx_avr_get_battery_voltage();
bool nx_avr_battery_is_aa();
U32 nx_avr_get_sensor_value(U32 sensor);
void nx_avr_get_version(U8 *major, U8 *minor);

void nx_avr_set_motor(U32 motor, int power_percent, bool brake);
void nx_avr_set_input_power(U32 sensor, U32 power_type);

#endif
