#ifndef __NXOS_RADAR_H__
#define __NXOS_RADAR_H__

#include "base/types.h"

/* As defined in the NXT Hardware Developer Kit, the Ultrasonic sensor
 * has been given address 1 (within a 7 bit context).
 */
#define RADAR_I2C_ADDRESS  0x01

#define RADAR_DEFAULT_INTERVAL 1

/* Radar operation modes: these are values that can be set at the
 * RADAR_OP_MODE (see below) to change the radar's behavior.
 */
#define RADAR_OP_OFF       0x00
#define RADAR_OP_SINGLE    0x01
#define RADAR_OP_CONTINOUS 0x02
#define RADAR_OP_EVENT     0x03
#define RADAR_OP_RESET     0x04

void nx_radar_init(U8 sensor);
bool nx_radar_detect(U8 sensor);
void nx_radar_reset(U8 sensor);
void nx_radar_info(U8 sensor);

/** Getters for static values. */
bool nx_radar_get_version(U8 sensor, U8 *version);
bool nx_radar_get_product_id(U8 sensor, U8 *product_id);
bool nx_radar_get_sensor_type(U8 sensor, U8 *sensor_type);
U8 nx_radar_get_factory_zero(U8 sensor);
U8 nx_radar_get_factory_scale_factor(U8 sensor);
U8 nx_radar_get_factory_scale_divisor(U8 sensor);
bool nx_radar_get_measurement_units(U8 sensor, U8 *units);

/** Getters for dynamic values and measurements */
U8 nx_radar_get_interval(U8 sensor);
U8 nx_radar_get_op_mode(U8 sensor);
U8 nx_radar_read_distance(U8 sensor, S8 object);
bool nx_radar_read_all(U8 sensor, U8 *buf);
U8 nx_radar_get_current_zero(U8 sensor);
U8 nx_radar_get_current_scale_factor(U8 sensor);
U8 nx_radar_get_current_scale_divisor(U8 sensor);

/** Setters to configure the radar. */
bool nx_radar_set_interval(U8 sensor, U8 interval);
bool nx_radar_set_op_mode(U8 sensor, U8 mode);
bool nx_radar_set_current_zero(U8 sensor, U8 zero);
bool nx_radar_set_current_scale_factor(U8 sensor, U8 factor);
bool nx_radar_set_current_scale_divisor(U8 sensor, U8 divisor);

#endif
