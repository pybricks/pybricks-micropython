/* Driver for the NXT ultrasonic radar.
 *
 * Among the functionnalities provided by this sensor, this driver
 * supports the following features:
 *
 *  - Continuous measurements 0-7
 *  - Measurement interval management
 *  - Warm reset to factory defaults
 */

#include "base/at91sam7s256.h"

#include "base/types.h"
#include "base/nxt.h"
#include "base/interrupts.h"
#include "base/display.h"
#include "base/util.h"
#include "base/drivers/i2c_memory.h"
#include "base/drivers/radar.h"

/* LEGO's Radar factory sensor type. */
#define RADAR_LEGO_SENSOR_TYPE "Sonar"

/** Radar commands.
 *
 * This array of structures is mapped by the radar_memory_slots enum
 * (see radar.h) and links the address in the radar's memory for the given
 * value to its returned data length.
 */
static struct radar_cmd_info {
  U8 addr;
  U8 len;
} radar_cmds[RADAR_N_COMMANDS] = {
  { 0x00, 8 }, // RADAR_VERSION: V1.0
  { 0x08, 8 }, // RADAR_PRODUCT_ID: LEGO
  { 0x10, 8 }, // RADAR_SENSOR_TYPE: Sonar
  { 0x11, 1 }, // RADAR_FACTORY_ZERO: 0x00
  { 0x12, 1 }, // RADAR_FACTORY_SCALE_FACTOR: 0x01
  { 0x13, 1 }, // RADAR_FACTORY_SCALE_DIVISOR: 0x0E
  { 0x14, 7 }, // RADAR_MEASUREMENT_UNITS: 10E-2m

  { 0x40, 1 }, // RADAR_INTERVAL: Interval
  { 0x41, 1 }, // RADAR_OP_MODE: See radar_mode

  // RADAR_R0 ... RADAR_R7: Radar readings
  { 0x42, 1 }, { 0x43, 1 }, { 0x44, 1 }, { 0x45, 1 },
  { 0x46, 1 }, { 0x47, 1 }, { 0x48, 1 }, { 0x49, 1 },

  { 0x50, 1 }, // RADAR_CURRENT_ZERO: Zero
  { 0x51, 1 }, // RADAR_CURRENT_SCALE_FACTOR: Scale factor
  { 0x52, 1 }, // RADAR_CURRENT_SCALE_DIVISOR: Scale divisor
};

/** Initializes the radar sensor in LEGO compatibility mode. */
void nx_radar_init(U32 sensor) {
  nx_i2c_memory_init(sensor, RADAR_I2C_ADDRESS, TRUE);
}

/** Close the radar and disable the sensor port. */
void nx_radar_close(U32 sensor) {
  nx_i2c_memory_close(sensor);
}

/** Reads a value from the radar's memory slot into the provided buffer. */
bool nx_radar_read(U32 sensor, radar_memory_slot slot, U8 *val) {
  struct radar_cmd_info *cmd = &radar_cmds[slot];
  return nx_i2c_memory_read(sensor, cmd->addr, val, cmd->len) == I2C_ERR_OK;
}

/** Returns a value from the radar's memory.
 *
 * Defaults to 0xFF if the read failed.
 */
U8 nx_radar_read_value(U32 sensor, radar_memory_slot slot) {
  U8 value;
  return nx_radar_read(sensor, slot, &value) ? value : 0xFF;
}

/** Writes the given buffer into the radar's memory slot. */
bool nx_radar_write(U32 sensor, radar_memory_slot slot, U8 *val) {
  struct radar_cmd_info *cmd = &radar_cmds[slot];
  return nx_i2c_memory_write(sensor, cmd->addr, val, cmd->len) == I2C_ERR_OK;
}

/** Radar detection.
 *
 * Tries to detect the presence of a compatible radar on the given port by
 * reading the device's sensor type and comparing it to the default sensor
 * type of the LEGO-manufactured radar, "Sonar".
 *
 * Returns TRUE if a compatible radar was found.
 */
bool nx_radar_detect(U32 sensor) {
  U8 type[8] = { 0x0 };

  return nx_radar_read(sensor, RADAR_SENSOR_TYPE, type)
    && streq((char *)type, RADAR_LEGO_SENSOR_TYPE);
}

/** High-level radar control interface. */

/** Resets the radar connected to the given sensor port.
 *
 * Requests a warm radar reset, and reset all parameters to factory
 * defaults.
 */
void nx_radar_reset(U32 sensor) {
  U8 reset = RADAR_MODE_RESET;
  U8 val = 0x0;

  /* Do a warm reset. */
  nx_radar_write(sensor, RADAR_OP_MODE, &reset);

  /* Reset zero, scale factor and scale divisor to factory values. */
  if (nx_radar_read(sensor, RADAR_FACTORY_ZERO, &val)) {
    nx_radar_write(sensor, RADAR_CURRENT_ZERO, &val);
  }

  if (nx_radar_read(sensor, RADAR_FACTORY_SCALE_FACTOR, &val)) {
    nx_radar_write(sensor, RADAR_CURRENT_SCALE_FACTOR, &val);
  }

  if (nx_radar_read(sensor, RADAR_FACTORY_SCALE_DIVISOR, &val)) {
    nx_radar_write(sensor, RADAR_CURRENT_SCALE_DIVISOR, &val);
  }

  val = RADAR_DEFAULT_INTERVAL;
  nx_radar_write(sensor, RADAR_INTERVAL, &val);
}

/** Returns radar's measurement #object from the given sensor number.
 *
 * To emit a beep which freq ranges from approx 8 kHz for smallest distances
 * to 380 Hz for greater detected distances (up to ??cm), you can use:
 *   sound_freq_async(8000 - r0 * 30, 100);
 *
 * Note: a return value of 0x00 means that no object was detected. A value
 * of 0xFF means that the read failed.
 */
U8 nx_radar_read_distance(U32 sensor, U32 object) {
  if (object > RADAR_MAX_OBJECT_NUMBER-1)
    return 0xFF;

  return nx_radar_read_value(sensor, RADAR_R0+object);
}

/** Read all radar's measurements at once.
 *
 * The given buffer 'buf' must be pre-allocated to hold the eight measurements
 * of the radar.
 *
 * Note: DOES NOT WORK (is it even possible to read as many bytes we want?)
 */
bool nx_radar_read_all(U32 sensor, U8 *buf) {
  /* We use the low-level i2c_memory_read function here to try to read
   * all measurements at once.
   */
  return nx_i2c_memory_read(sensor, radar_cmds[RADAR_R0].addr,
			    buf, 8) == I2C_ERR_OK;
}

/** Sets the radar continuous measurement interval. */
bool nx_radar_set_interval(U32 sensor, U8 interval) {
  return nx_radar_write(sensor, RADAR_INTERVAL, &interval);
}

/** Sets the radar operation mode. See radar.h for available modes. */
bool nx_radar_set_op_mode(U32 sensor, radar_op_mode mode) {
  U8 val = mode;
  return nx_radar_write(sensor, RADAR_OP_MODE, &val);
}

/** Display connected radar's information. */
void nx_radar_info(U32 sensor) {
  U8 buf[8];

  // Product ID (LEGO)
  memset(buf, 0, 8);
  nx_radar_read(sensor, RADAR_PRODUCT_ID, buf);
  nx_display_string((char *)buf);
  nx_display_string(" ");

  // Sensor Type (Sonar)
  memset(buf, 0, 8);
  nx_radar_read(sensor, RADAR_SENSOR_TYPE, buf);
  nx_display_string((char *)buf);
  nx_display_string(" ");

  // Version (V1.0)
  memset(buf, 0, 8);
  nx_radar_read(sensor, RADAR_VERSION, buf);
  nx_display_string((char *)buf);
  nx_display_end_line();

  // Measurement units
  nx_display_string("Units: ");
  memset(buf, 0, 8);
  nx_radar_read(sensor, RADAR_MEASUREMENT_UNITS, buf);
  nx_display_string((char *)buf);
  nx_display_end_line();

  // Measurement interval
  nx_display_string("Interval: ");
  nx_display_uint(nx_radar_read_value(sensor, RADAR_INTERVAL));
  nx_display_string(" ms?\n");
}
