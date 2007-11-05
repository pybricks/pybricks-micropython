/** @file radar.h
 *  @brief Ultrasonic radar driver.
 *
 * Driver for NXT Ultrasonic radars acting as remote I2C memory units.
 */

/* Copyright (C) 2007 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */
 
#ifndef __NXOS_RADAR_H__
#define __NXOS_RADAR_H__

#include "base/types.h"

/* As defined in the NXT Hardware Developer Kit, the Ultrasonic sensor
 * has been given address 1 (within a 7 bit context).
 */
#define RADAR_I2C_ADDRESS  0x01

/** Radar's internal memory addresses.
 *
 * This enum contains the radar's internal memory addresses of the
 * radar parameters and readings.
 */
typedef enum {
  RADAR_VERSION,
  RADAR_PRODUCT_ID,
  RADAR_SENSOR_TYPE,
  RADAR_FACTORY_ZERO,
  RADAR_FACTORY_SCALE_FACTOR,
  RADAR_FACTORY_SCALE_DIVISOR,
  RADAR_MEASUREMENT_UNITS,

  RADAR_INTERVAL,
  RADAR_OP_MODE,
  RADAR_R0,
  RADAR_R1,
  RADAR_R2,
  RADAR_R3,
  RADAR_R4,
  RADAR_R5,
  RADAR_R6,
  RADAR_R7,
  RADAR_CURRENT_ZERO,
  RADAR_CURRENT_SCALE_FACTOR,
  RADAR_CURRENT_SCALE_DIVISOR,
  
  RADAR_N_COMMANDS,
} radar_memory_slot;

/* Radar default continuous measurement interval value. */
#define RADAR_DEFAULT_INTERVAL 1

/* Radar operation modes: these are values that can be set at the
 * RADAR_OP_MODE (see below) to change the radar's behavior.
 */
#define RADAR_OP_OFF       0x00
#define RADAR_OP_SINGLE    0x01
#define RADAR_OP_CONTINOUS 0x02
#define RADAR_OP_EVENT     0x03
#define RADAR_OP_RESET     0x04

void nx_radar_init(U32 sensor);
bool nx_radar_detect(U32 sensor);
void nx_radar_reset(U32 sensor);
void nx_radar_info(U32 sensor);

bool nx_radar_read(U32 sensor, radar_memory_slot slot, U8 *buf);
bool nx_radar_write(U32 sensor, radar_memory_slot slot, U8 *val);
U8 nx_radar_read_value(U32 sensor, radar_memory_slot slot);

U8 nx_radar_read_distance(U32 sensor, U32 object);
bool nx_radar_read_all(U32 sensor, U8 *buf);
bool nx_radar_set_interval(U32 sensor, U8 interval);
bool nx_radar_set_op_mode(U32 sensor, U8 mode);

#endif /* __NXOS_RADAR_H__ */
