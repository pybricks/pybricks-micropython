// tary, 1:42 2013/4/28
#include "am18x_lib.h"
#include "i2c_inf.h"

// AM1808 I2C pins
#define I2C0_SCL			4,8,2
#define I2C0_SDA			4,12,2
#define I2C1_SCL			4,16,4
#define I2C1_SDA			4,20,4

static i2c_conf_t iconf_buf[] = {
{
	100000,
	0,
	I2C_BITMODE_7BIT,
	I2C_OPERMODE_Master_transmitter,
},
{
	100000,
	0,
	I2C_BITMODE_7BIT,
	I2C_OPERMODE_Master_transmitter,
},
};

static i2c_conf_t* bus2conf(I2C_con_t* bus) {
	if (bus == I2C0) {
		return &iconf_buf[0];
	}
	else if (bus == I2C1) {
		return &iconf_buf[1];
	}
	return NULL;
}

int i2c_init(I2C_con_t* bus, uint32_t freq) {
	if (bus == I2C0) {
		syscfg_pinmux(I2C0_SCL);
		syscfg_pinmux(I2C0_SDA);
	} else
	if (bus == I2C1) {
		psc_state_transition(PSC_I2C1, PSC_STATE_ENABLE);
		syscfg_pinmux(I2C1_SCL);
		syscfg_pinmux(I2C1_SDA);
	}
	if (bus == I2C0 || bus == I2C1) {
		bus2conf(bus)->freq = freq;
	}
	return 0;
}

int i2c_read(I2C_con_t* bus, uint16_t dev, uint8_t* bytes, uint32_t cnt) {
	uint32_t i;
	i2c_conf_t* iconf = bus2conf(bus);

	iconf->opermode = I2C_OPERMODE_Master_receiver;
	iconf->addr = dev;
	i2c_conf(bus, iconf);

	i2c_cmd(bus, I2C_CMD_SET_CNT, cnt);
	i2c_cmd(bus, I2C_CMD_WAIT_FREE, 0);
	i2c_cmd(bus, I2C_CMD_BUS_START, 0);
	i2c_cmd(bus, I2C_CMD_WAIT_BUSY, 0);

	for (i = 0; i < cnt; i++) {
		// dump_regs_word("I2C0", (uint32_t)bus, sizeof(I2C_con_t));
		i2c_cmd(bus, I2C_CMD_WAIT_RX, 0);
		bytes[i] = i2c_get_rx(bus);
	}

	i2c_cmd(bus, I2C_CMD_WAIT_ARDY, 0);
	i2c_cmd(bus, I2C_CMD_BUS_STOP, 0);
	i2c_cmd(bus, I2C_CMD_WAIT_SLAVE, 0);

	return 0;
}

int i2c_write(I2C_con_t* bus, uint16_t dev, cuint8_t* bytes, uint32_t cnt) {
	uint32_t i;
	i2c_conf_t* iconf = bus2conf(bus);

	iconf->opermode = I2C_OPERMODE_Master_transmitter;
	iconf->addr = dev;
	i2c_conf(bus, iconf);

	i2c_cmd(bus, I2C_CMD_SET_CNT, cnt);
	i2c_cmd(bus, I2C_CMD_WAIT_FREE, 0);
	i2c_cmd(bus, I2C_CMD_BUS_START, 0);
	i2c_cmd(bus, I2C_CMD_WAIT_BUSY, 0);

	for (i = 0; i < cnt; i++) {
		// dump_regs_word("I2C0", (uint32_t)bus, sizeof(I2C_con_t));
		i2c_cmd(bus, I2C_CMD_WAIT_TX, 0);
		i2c_set_tx(bus, bytes[i]);
	}

	i2c_cmd(bus, I2C_CMD_BUS_STOP, 0);
	i2c_cmd(bus, I2C_CMD_WAIT_SLAVE, 0);

	return 0;
}
