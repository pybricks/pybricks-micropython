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

/** @addtogroup driver */
/*@{*/

/** @defgroup radar Ultrasonic radar driver
 *
 * The radar driver provides a simple API to use and control the LEGO
 * Ultrasonic sensor. This sensor can provide up to 8 readings of the
 * distances to objects in the vicinity. Although the quality of the
 * radar readings may be altered if other ultrasonic radars are nearby,
 * the readings have a very good precision.
 */
/*@{*/

/** LEGO Ultrasonic sensor I2C address (as defined in the NXT Hardware
 * Developer Kit).
 */
#define RADAR_I2C_ADDRESS  0x01

/** Radar internal memory addresses.
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

/** Radar operation modes.
 *
 * This enum defines the operation modes supported by the radar.
 */
typedef enum {
  RADAR_MODE_OFF = 0x00,
  RADAR_MODE_SINGLE_SHOT = 0x01,
  RADAR_MODE_CONTINUOUS = 0x02,
  RADAR_MODE_EVENT_CAPTURE = 0x03,
  RADAR_MODE_RESET = 0x04,
} radar_op_mode;

/** Radar default continuous measurement interval value. */
#define RADAR_DEFAULT_INTERVAL 1

/** Maximum number of objects detected by the radar. */
#define RADAR_MAX_OBJECT_NUMBER 8

/** Initialize the radar on port @a sensor.
 *
 * @param sensor The sensor port number.
 */
void nx_radar_init(U32 sensor);

/** Check the presence of a compatible radar on port @a sensor.
 *
 * @param sensor The sensor port number.
 *
 * @note The device on port @a sensor first needs to be initialized with
 * nx_radar_init().
 *
 * @return True if a compatible radar was found, false otherwise.
 */
bool nx_radar_detect(U32 sensor);

/** Reset the radar connected on port @a sensor.
 *
 * Requests a warm reset from the radar and reset the radar configuration
 * to its factory defaults.
 *
 * @param sensor The sensor port number.
 */
void nx_radar_reset(U32 sensor);

/** Display the radar's information.
 *
 * Displays on the NXT screen the radar's information, including the
 * product ID, sensor type, device version, measurement units and
 * the measurements interval.
 *
 * @param sensor The sensor port number.
 */
void nx_radar_info(U32 sensor);

/** Read from the radar's memory.
 *
 * @param sensor The sensor port number.
 * @param slot The memory slot to read from (see radar_memory_slot).
 * @param buf A buffer to hold the read data.
 *
 * @note The buffer @a buf must be pre-allocated to hold the data read from
 * the radar.
 *
 * @return Returns true if the read succeeded.
 */
bool nx_radar_read(U32 sensor, radar_memory_slot slot, U8 *buf);

/** Write to the radar's memory.
 *
 * @param sensor The sensor port number.
 * @param slot The memory slot to write to.
 * @param val The data to write at @a slot.
 *
 * @warning This function directly writes to the radar without any additional
 * check. Use with caution. To manipulate the device, use the higher-level
 * API functions when they exist.
 *
 * @return Returns true if the write succeeded.
 */
bool nx_radar_write(U32 sensor, radar_memory_slot slot, U8 *val);

/** Read a single-byte value from the radar's memory.
 *
 * @param sensor The sensor port number.
 * @param slot The memory slot to read from.
 *
 * @note The returned value defaults to 0xFF if the read failed.
 *
 * @return The value of the requested @a slot.
 */
U8 nx_radar_read_value(U32 sensor, radar_memory_slot slot);

/** Read a radar's measurement.
 *
 * @param sensor The sensor port number.
 * @param object The object ID (0..RADAR_MAX_OBJECT_NUMBER-1).
 *
 * @return The distance to the objects, in the measurement units of the radar.
 */
U8 nx_radar_read_distance(U32 sensor, U32 object);

/** Read all radar measurement at once (not working).
 *
 * @param sensor The sensor port number.
 * @param buf A buffer to hold the measurements.
 *
 * @note The buffer @a buf must be pre-allocated to hold all the radar
 * measurements (RADAR_MAX_OBJECT_NUMBER objects).
 *
 * @return Return true if the read succeeded.
 */
bool nx_radar_read_all(U32 sensor, U8 *buf);

/** Set the radar measurement interval to @a interval.
 *
 * @param sensor The sensor port number.
 * @param interval The new measurement interval.
 *
 * @return Returns true if the operation succeeded.
 */
bool nx_radar_set_interval(U32 sensor, U8 interval);

/** Set the radar operation mode to @a mode.
 *
 * @param sensor The sensor port number.
 * @param mode The new operation mode for the radar.
 *
 * @return Returns true if the operation succeeded.
 */
bool nx_radar_set_op_mode(U32 sensor, radar_op_mode mode);

/*@}*/
/*@}*/

#endif /* __NXOS_RADAR_H__ */
