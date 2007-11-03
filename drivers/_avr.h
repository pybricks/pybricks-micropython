#ifndef __NXOS_BASE_DRIVERS__AVR_H__
#define __NXOS_BASE_DRIVERS__AVR_H__

#include "base/drivers/avr.h"

void nx__avr_init();
void nx__avr_fast_update();
U32 nx__avr_get_sensor_value(U32 sensor);
void nx__avr_set_motor(U32 motor, int power_percent, bool brake);
void nx__avr_power_down();
void nx__avr_firmware_update_mode();

#endif
