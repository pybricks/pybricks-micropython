#ifndef __NXOS_AVR_H__
#define __NXOS_AVR_H__

#include "base/types.h"

typedef enum {
  BUTTON_NONE = 0,
  BUTTON_OK,
  BUTTON_CANCEL,
  BUTTON_LEFT,
  BUTTON_RIGHT,
} avr_button_t;

avr_button_t nx_avr_get_button();
U32 nx_avr_get_battery_voltage();
bool nx_avr_battery_is_aa();
void nx_avr_get_version(U8 *major, U8 *minor);

#endif
