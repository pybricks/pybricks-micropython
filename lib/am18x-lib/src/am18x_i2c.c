// tary, 22:02 2013/3/13
#include "am18x_i2c.h"
#include "am18x_dclk.h"

// 22.2.2 Clock Generation
#define PrescaledModuleClockLow			(6700000UL)
#define PrescaledModuleClockHigh		(13300000UL)

// static inline
uint32_t i2c_input_freq(I2C_con_t* icon) {
	if (icon == I2C0) {
		return dev_get_freq(DCLK_ID_I2C0);
	}
	if (icon == I2C1) {
		return dev_get_freq(DCLK_ID_I2C1);
	}
	return 0UL;
}

static inline uint32_t ipsc_to_d(uint32_t ipsc) {
	uint32_t d;

	if (ipsc == 0) {
		d = 7;
	} else if (ipsc == 1) {
		d = 6;
	} else {
		d = 5;
	}
	return d;
}

uint32_t i2c_get_serial_clock(I2C_con_t* icon) {
	uint32_t freq, scale_freq, ipsc, divider;

	freq = i2c_input_freq(icon);

	ipsc = FIELD_XGET(icon->ICPSC, ICPSC_IPSC_MASK);

	divider = (ipsc_to_d(ipsc) << 1);
	divider += FIELD_XGET(icon->ICCLKL, ICCLKL_MASK);
	divider += FIELD_XGET(icon->ICCLKH, ICCLKH_MASK);

	scale_freq = freq / (ipsc + 1);

	return scale_freq / divider;
}

#define I2C_RESET(ic)	{ ic->ICMDR = FIELD_SET(ic->ICMDR, ICMDR_IRS_MASK, ICMDR_IRS_reset); }
#define I2C_UNRESET(ic) { ic->ICMDR = FIELD_SET(ic->ICMDR, ICMDR_IRS_MASK, ICMDR_IRS_none); }

am18x_rt i2c_set_serial_clock(I2C_con_t* icon, uint32_t freq) {
	uint32_t ipsc, scale_freq;
	// uint32_t msk;

	if (icon == I2C0) {
		ipsc = 1;
	} else {
		ipsc = 0;
	}

	scale_freq = i2c_input_freq(icon) / (ipsc + 1);

	// msk = ICMDR_IRS_MASK;
	// icon->ICMDR = FIELD_SET(icon->ICMDR, msk, ICMDR_IRS_reset);

	icon->ICPSC = FIELD_SET(0, ICPSC_IPSC_MASK, ICPSC_IPSC_VAL(ipsc));

	// 22.2.2 Clock Generation
	// I2C serial clock frequency = prescaled module clock frequency
	//                              / ( ICCL + d ) + ( ICCH + d )
	icon->ICCLKL = scale_freq / (freq * 2) - ipsc_to_d(ipsc);
	icon->ICCLKH = icon->ICCLKL;

	// icon->ICMDR = FIELD_SET(icon->ICMDR, msk, ICMDR_IRS_none);

	return AM18X_OK;
}

am18x_rt i2c_conf(I2C_con_t* icon, i2c_conf_t* conf) {
	uint32_t reg, msk, v;

	// 22.2.11.1 Configuring the I2C in Master Receiver Mode
	// and Servicing Receive Data via CPU

	// 1. Enable I2C Clock from Power and Sleep Controller,
	// if it is driven by the Power and Sleep Controller

	// 2. Place I2C in reset
	I2C_RESET(icon);

	// 3. Configure ICMDR
	reg = icon->ICMDR;
	msk = v = 0;

	// Configure I2C as Master
	msk |= ICMDR_MST_MASK;
	if (conf->opermode == I2C_OPERMODE_Master_receiver
	||  conf->opermode == I2C_OPERMODE_Master_transmitter) {
		v |= ICMDR_MST_master;
	} else {
		v |= ICMDR_MST_slave;
	}

	// Indicate the I2C configuration to be used
	msk |= ICMDR_TRX_MASK;
	if (conf->opermode == I2C_OPERMODE_Slave_transmitter
	||  conf->opermode == I2C_OPERMODE_Master_transmitter) {
		v |= ICMDR_TRX_transmitter;
	} else {
		v |= ICMDR_TRX_receiver;
	}

	// Indicate 7-bit addressing is to be used
	msk |= ICMDR_XA_MASK;
	if (conf->bitmode == I2C_BITMODE_7BIT) {
		v |= ICMDR_XA_7bit;
	} else if (conf->bitmode == I2C_BITMODE_10BIT) {
		v |= ICMDR_XA_10bit;
	}

	// Disable repeat mode
	msk |= ICMDR_RM_MASK;
	v |= ICMDR_RM_no;

	// Disable loopback mode
	msk |= ICMDR_DLB_MASK;
	v |= ICMDR_DLB_disable;

	// Disable free data format
	msk |= ICMDR_FDF_MASK;
	if (conf->bitmode == I2C_BITMODE_FreeDataFormat) {
		v |= ICMDR_FDF_yes;
	} else {
		v |= ICMDR_FDF_no;
	}

	// Disable start byte mode 
	// if addressing a fully fledged I2C device
	msk |= ICMDR_STB_MASK;
	v |= ICMDR_STB_no;

	// Set number of bits to fransfer to be 8 bits
	msk |= ICMDR_BC_MASK;
	v |= ICMDR_BC_VAL(8);

	icon->ICMDR = FIELD_SET(reg, msk, v);

	// 4. Configure Slave Address: the I2C device
	// this I2C master would be addressing
	if (conf->opermode == I2C_OPERMODE_Master_receiver
	||  conf->opermode == I2C_OPERMODE_Master_transmitter) {
		msk = ICSAR_SADDR_MASK;
		icon->ICSAR = FIELD_SET(0, msk, ICSAR_SADDR_VAL(conf->addr));
	} else {
		// Set i2c own address, if it's as slave device
		msk = ICOAR_OADDR_MASK;
		icon->ICOAR = FIELD_SET(0, msk, ICOAR_OADDR_VAL(conf->addr));
	}

	// 5. Configure the peripheral clock operation frequency.
	// 6. Configure I2C master clock frequency
	i2c_set_serial_clock(icon, conf->freq);

	// 7. Make sure the interrupt status register (ICSTR) is cleared
	icon->ICSTR = icon->ICSTR;
	while (icon->ICIVR != 0);

	// 8. Take I2C controller out of reset
	I2C_UNRESET(icon);

	reg = icon->ICMDR;
	icon->ICMDR = FIELD_SET(reg, ICMDR_FREE_MASK, ICMDR_FREE_run);

	return AM18X_OK;
}

am18x_rt i2c_cmd(I2C_con_t* icon, uint32_t cmd, uint32_t arg) {
	uint32_t reg;

	switch (cmd) {
	case I2C_CMD_WAIT_FREE:
		// 9. Wait until bus busy bit is cleared
		while (FIELD_GET(icon->ICSTR, ICSTR_BB_MASK) != ICSTR_BB_free);
		break;

	case I2C_CMD_WAIT_BUSY:
		while (FIELD_GET(icon->ICSTR, ICSTR_BB_MASK) != ICSTR_BB_busy);
		break;

	case I2C_CMD_WAIT_SLAVE:
		while (FIELD_GET(icon->ICMDR, ICMDR_MST_MASK) != ICMDR_MST_slave);
		break;

	case I2C_CMD_WAIT_ARDY:
		while (FIELD_GET(icon->ICSTR, ICSTR_ARDY_MASK) != ICSTR_ARDY_ready);
		break;

	case I2C_CMD_BUS_START:
		// 10. Generate a START event, followed by Slave Address, etc.
		reg = icon->ICMDR;
		icon->ICMDR = FIELD_SET(reg, ICMDR_STT_MASK, ICMDR_STT_yes);
		break;

	case I2C_CMD_WAIT_RX:
		// 11. Wait until data is received
		while (FIELD_GET(icon->ICSTR, ICSTR_ICRRDY_MASK) != ICSTR_ICRRDY_ready);
		break;

	case I2C_CMD_WAIT_TX:
		while (FIELD_GET(icon->ICSTR, ICSTR_ICXRDY_MASK) != ICSTR_ICXRDY_ready);
		break;

	case I2C_CMD_CLR_RX:
		icon->ICSTR = FIELD_SET(icon->ICSTR, ICSTR_ICRRDY_MASK, ICSTR_ICRRDY_clear);
		break;

	case I2C_CMD_CLR_TX:
		icon->ICSTR = FIELD_SET(icon->ICSTR, ICSTR_ICXRDY_MASK, ICSTR_ICXRDY_clear);
		break;		

	case I2C_CMD_BUS_NACK:
		// 13. Configure the I2C controller not to generate an ACK on
		// the next/final byte reception
		reg = icon->ICMDR;
		icon->ICMDR = FIELD_SET(reg, ICMDR_NACKMOD_MASK, ICMDR_NACKMOD_yes);
		break;

	case I2C_CMD_BUS_STOP:
		// 14. End transer/release bus when transfer is done.
		// Generate a STOP event
		reg = icon->ICMDR;
		icon->ICMDR = FIELD_SET(reg, ICMDR_STP_MASK, ICMDR_STP_yes);
		break;

	case I2C_CMD_SET_CNT:
		icon->ICCNT = FIELD_SET(0, ICCNT_MASK, arg);
		break;

	case I2C_CMD_SET_REPEAT:
		I2C_RESET(icon);
		reg = icon->ICMDR;
		icon->ICMDR = FIELD_SET(reg, ICMDR_RM_MASK, ICMDR_RM_yes);
		I2C_UNRESET(icon);
		break;

	case I2C_CMD_CLR_REPEAT:
		I2C_RESET(icon);
		reg = icon->ICMDR;
		icon->ICMDR = FIELD_SET(reg, ICMDR_RM_MASK, ICMDR_RM_no);
		I2C_UNRESET(icon);
		break;

	case I2C_CMD_TRANSMITTER:
		I2C_RESET(icon);
		reg = icon->ICMDR;
		icon->ICMDR = FIELD_SET(reg, ICMDR_TRX_MASK, ICMDR_TRX_transmitter);
		I2C_UNRESET(icon);
		break;

	case I2C_CMD_RECEIVER:
		I2C_RESET(icon);
		reg = icon->ICMDR;
		icon->ICMDR = FIELD_SET(reg, ICMDR_TRX_MASK, ICMDR_TRX_receiver);
		I2C_UNRESET(icon);
		break;

	default:
		break;
	}
	return AM18X_OK;
}
