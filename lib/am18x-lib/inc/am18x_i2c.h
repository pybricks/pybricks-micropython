// tary, 22:00 2013/3/13

#ifndef __AM18X_I2C_H__
#define __AM18X_I2C_H__

#include "am18x_map.h"

typedef enum {
	I2C_BITMODE_7BIT,
	I2C_BITMODE_10BIT,
	I2C_BITMODE_FreeDataFormat,
} i2c_bitmode_t;

// 22.2.7 Operating Modes
typedef enum {
	I2C_OPERMODE_Slave_receiver,
	I2C_OPERMODE_Slave_transmitter,
	I2C_OPERMODE_Master_receiver,
	I2C_OPERMODE_Master_transmitter,
} i2c_opermode_t;

typedef enum {
	I2C_CMD_WAIT_FREE,
	I2C_CMD_WAIT_BUSY,
	I2C_CMD_WAIT_SLAVE,
	I2C_CMD_WAIT_ARDY,
	I2C_CMD_WAIT_RX,
	I2C_CMD_WAIT_TX,
	I2C_CMD_CLR_RX,
	I2C_CMD_CLR_TX,
	// auto clear
	I2C_CMD_BUS_START,
	// auto clear
	I2C_CMD_BUS_STOP,
	// auto clear
	I2C_CMD_BUS_NACK,
	I2C_CMD_SET_CNT,
	I2C_CMD_SET_REPEAT,
	I2C_CMD_CLR_REPEAT,
	I2C_CMD_TRANSMITTER,
	I2C_CMD_RECEIVER,
} i2c_cmd_t;

typedef struct {
	uint32_t	freq;
	uint16_t	addr;
	uint8_t		bitmode;
	uint8_t		opermode;
} i2c_conf_t;

uint32_t i2c_get_serial_clock(I2C_con_t* icon);
am18x_rt i2c_set_serial_clock(I2C_con_t* icon, uint32_t freq);
am18x_rt i2c_conf(I2C_con_t* icon, i2c_conf_t* conf);
static inline int32_t i2c_get_rx(I2C_con_t* icon) {
	return icon->ICDRR;
}
static inline am18x_rt i2c_set_tx(I2C_con_t* icon, uint8_t data) {
	icon->ICDXR = data;
	return AM18X_TRUE;
}
am18x_rt i2c_cmd(I2C_con_t* icon, uint32_t cmd, uint32_t arg);

#endif//__AM18X_I2C_H__
