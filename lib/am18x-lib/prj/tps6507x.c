// tary, 1:49 2013/4/28
#include "am18x_lib.h"
#include "tps6507x.h"
#include "auxlib.h"
#include "i2c_inf.h"

// 1015640A_AM1808_SOM-M1_Schematic.pdf
// Page 7, PMIC
#define TPS6507X_BUS			I2C0

// slvs950f.pdf
// Device Address
#define TPS6507X_ADDR			0x48

#define TPS6507X_SPEED			350000	// 400K

// REGISTERS
typedef enum {
	TPS6507X_REG_NULL,		// 0x00
	TPS6507X_REG_PPATH1,
	TPS6507X_REG_INT,
	TPS6507X_REG_CHGCONFIG0,

	TPS6507X_REG_CHGCONFIG1,	// 0x04
	TPS6507X_REG_CHGCONFIG2,
	TPS6507X_REG_CHGCONFIG3,
#define ADCONFIG_SELECT_MASK		0x0FUL
#define ADCONFIG_SELECT_ac		0x06UL
#define ADCONFIG_SELECT_sys		0x07UL
	TPS6507X_REG_ADCONFIG,

	TPS6507X_REG_TSCMODE,		// 0x08
	TPS6507X_REG_ADRESULT_1,
	TPS6507X_REG_ADRESULT_2,
	TPS6507X_REG_PGOOD,

	TPS6507X_REG_PGOODMASK,		// 0x0C
	TPS6507X_REG_CON_CTRL1,
	TPS6507X_REG_CON_CTRL2,
	TPS6507X_REG_CON_CTRL3,

// apply to DEFDCDC1, DEFDCDC2_LOW, DEFDCDC2_HIGH
// DEFDCDC3_LOW, DEFDCDC3_HIGH, DEFLDO2
#define DEFDCDCX_OUTVOLT_MASK		0x3FUL
	TPS6507X_REG_DEFDCDC1,		// 0x10
	TPS6507X_REG_DEFDCDC2_LOW,
	TPS6507X_REG_DEFDCDC2_HIGH,
	TPS6507X_REG_DEFDCDC3_LOW,

	TPS6507X_REG_DEFDCDC3_HIGH,	// 0x14
	TPS6507X_REG_DEFSLEW,
#define LDO_CTRL1_OUTVOLT_MASK		0x0FUL
	TPS6507X_REG_LDO_CTRL1,
	TPS6507X_REG_DEFLDO2,

	TPS6507X_REG_WLED_CTRL1,	// 0x18
	TPS6507X_REG_WLED_CTRL2,
	TPS6507X_REG_CNT,
} TPS6507X_reg_t;

enum {
	BIT_DEF(PPATH1,7,USBPresent,no,yes),
	BIT_DEF(PPATH1,6,ACPresent,no,yes),
	BIT_DEF(PPATH1,5,USBPower,enable,disable),
	BIT_DEF(PPATH1,4,ACPower,enable,disable),
	BIT_DEF(ADCONFIG,7,Enable,no,yes),
	BIT_DEF(ADCONFIG,6,Start,no,yes),
	BIT_DEF(ADCONFIG,5,End,no,yes),
	BIT_DEF(ADCONFIG,4,Vref,disable,enable),
	BIT_DEF(CON_CTRL1,4,DCDC1,disable,enable),
	BIT_DEF(CON_CTRL1,3,DCDC2,disable,enable),
	BIT_DEF(CON_CTRL1,2,DCDC3,disable,enable),
	BIT_DEF(CON_CTRL1,1,LDO1,disable,enable),
	BIT_DEF(CON_CTRL1,0,LDO2,disable,enable),
};

static const uint16_t dcdcx_voltage[] = {
	 725,  750,  775,  800,	// 725 + 25 * i
	 825,  850,  875,  900,
	 925,  950,  975, 1000,
	1025, 1050, 1075, 1100,
	1125, 1150, 1175, 1200,
	1225, 1250, 1275, 1300,
	1325, 1350, 1375, 1400,
	1425, 1450, 1475, 1500,

	1550, 1600, 1650, 1700, // 1550 + 50 * (i - 16)
	1750, 1800, 1850, 1900,
	1950, 2000, 2050, 2100,
	2150, 2200, 2250, 2300,
	2350, 2400, 2450, 2500,
	2550, 2600, 2650, 2700,
	2750, 2800, 2850, 2900,
	3000, 3100, 3200, 3300, // 3000 + 100 * (i - 28)
	0, 0,
};

static const uint16_t ldo1_voltage[] = {
	1000, 1100, 1200, 1250,
	1300, 1350, 1400, 1500,
	1600, 1800, 2500, 2750,
	2800, 3000, 3100, 3200,
	0, 0,
};

static inline int tps6507x_reg_write(uint8_t reg, uint8_t val) {
	uint8_t bytes[2];

	bytes[0] = reg;
	bytes[1] = val;

	i2c_write(TPS6507X_BUS, TPS6507X_ADDR, bytes, sizeof bytes);

	return 0;
}

static inline int tps6507x_reg_read(uint8_t reg) {
	uint8_t bytes[1];

	bytes[0] = reg;
	i2c_write(TPS6507X_BUS, TPS6507X_ADDR, bytes, 1);
	i2c_read(TPS6507X_BUS, TPS6507X_ADDR, bytes, sizeof bytes);

	return bytes[0];
}

int tps6507x_dump_regs(void) {
	int i;

	for (i = 1; i < TPS6507X_REG_CNT ; i++) {
		printk("[0x%.2X] = 0x%.2X\n", i, tps6507x_reg_read(i));
	}
	return 0;
}

int tps6507x_conf(void) {
	i2c_init(TPS6507X_BUS, TPS6507X_SPEED);
	return 0;
}

int tps6507x_get_adc(pwr_type_t pt) {
	uint32_t msk;
	uint16_t r;
	uint8_t v;

	if (pt != PWR_TYPE_AC
	 && pt != PWR_TYPE_SYS
	) {
		return -1;
	}

	// Set Bit AD ENABLE = 1 to provide power to the ADC
	msk = ADCONFIG_Enable_MASK;
	v = tps6507x_reg_read(TPS6507X_REG_ADCONFIG);
	v = FIELD_SET(v, msk, ADCONFIG_Enable_yes);
	tps6507x_reg_write(TPS6507X_REG_ADCONFIG, v);

	// Set input select for the ADC in register ADCONFIG to 011X
	v = tps6507x_reg_read(TPS6507X_REG_ADCONFIG);
	msk = ADCONFIG_SELECT_MASK;
	if (pt == PWR_TYPE_AC) {
		v = FIELD_SET(v, msk, ADCONFIG_SELECT_ac);
	} else {
		v = FIELD_SET(v, msk, ADCONFIG_SELECT_sys);
	}
	tps6507x_reg_write(TPS6507X_REG_ADCONFIG, v);

	// Start a conversion by setting CONVERSION START = 1;
	// wait until END OF CONVERSION = 1
	msk = ADCONFIG_Start_MASK;
	v = tps6507x_reg_read(TPS6507X_REG_ADCONFIG);
	v = FIELD_SET(v, msk, ADCONFIG_Start_yes);
	tps6507x_reg_write(TPS6507X_REG_ADCONFIG, v);

	msk = ADCONFIG_End_MASK;
	do {
		v = tps6507x_reg_read(TPS6507X_REG_ADCONFIG);
	} while (FIELD_GET(v, msk) != ADCONFIG_End_yes);

	// Read register ADRESULT_1 and ADRESULT_2
	r = tps6507x_reg_read(TPS6507X_REG_ADRESULT_1);
	v = tps6507x_reg_read(TPS6507X_REG_ADRESULT_2);
	r = __field_xset(r, 0x0300UL, v);

	return r;
}

static const uint16_t* pwrtyp2voltages(pwr_type_t pt, int* pr, uint8_t* pm) {
	const uint16_t* voltages;
	uint8_t msk;
	int reg;

	msk = DEFDCDCX_OUTVOLT_MASK;
	voltages = dcdcx_voltage;
	switch (pt) {
	case PWR_TYPE_DCDC1:
		reg = TPS6507X_REG_DEFDCDC1;
		break;
	case PWR_TYPE_DCDC2:
		reg = TPS6507X_REG_DEFDCDC2_HIGH;
		break;
	case PWR_TYPE_DCDC3:
		reg = TPS6507X_REG_DEFDCDC3_HIGH;
		break;
	case PWR_TYPE_LDO1:
		reg = TPS6507X_REG_LDO_CTRL1;
		msk = LDO_CTRL1_OUTVOLT_MASK;
		voltages = ldo1_voltage;
		break;
	case PWR_TYPE_LDO2:
		reg = TPS6507X_REG_DEFLDO2;
		break;
	default:
		return NULL;
	}
	if (pr != NULL) {
		*pr = reg;
	}
	if (pm != NULL) {
		*pm = msk;
	}
	return voltages;
}

int tps6507x_get_output(pwr_type_t pt) {
	const uint16_t* voltages;
	uint8_t msk;
	int reg, v;

	if ((voltages = pwrtyp2voltages(pt, &reg, &msk)) == NULL) {
		return 0;
	}

	v = tps6507x_reg_read(reg);
	v = __field_xget(v, msk);
	return voltages[v];
}

int tps6507x_set_output(pwr_type_t pt, uint16_t voltage) {
	const uint16_t* voltages;
	uint8_t msk;
	int i, reg, v;

	if ((voltages = pwrtyp2voltages(pt, &reg, &msk)) == NULL) {
		return -1;
	}

	for (i = 0; voltages[i] != 0 && voltages[i] <= voltage; i++) {
	}
	if (--i < 0) i = 0;

	v = tps6507x_reg_read(reg);
	v = __field_xset(v, msk, i);
	tps6507x_reg_write(reg, v);
	return 0;
}

int tps6507x_power_switch(pwr_type_t pt, am18x_bool on_noff) {
	uint32_t msk;
	uint8_t reg, v, f;

	if (pt == PWR_TYPE_AC) {
		reg = TPS6507X_REG_PPATH1;
		msk = PPATH1_ACPower_MASK;
		f = on_noff? PPATH1_ACPower_enable: PPATH1_ACPower_disable;
	} else if (pt == PWR_TYPE_USB) {
		reg = TPS6507X_REG_PPATH1;
		msk = PPATH1_USBPower_MASK;
		f = on_noff? PPATH1_USBPower_enable: PPATH1_USBPower_disable;
	} else if (pt == PWR_TYPE_DCDC1) {
		reg = TPS6507X_REG_CON_CTRL1;
		msk = CON_CTRL1_DCDC1_MASK;
		f = on_noff? CON_CTRL1_DCDC1_enable: CON_CTRL1_DCDC1_disable;
	} else if (pt == PWR_TYPE_LDO1) {
		reg = TPS6507X_REG_CON_CTRL1;
		msk = CON_CTRL1_LDO1_MASK;
		f = on_noff? CON_CTRL1_LDO1_enable: CON_CTRL1_LDO1_disable;
	} else if (pt == PWR_TYPE_LDO2
	  || pt == PWR_TYPE_DCDC2
	  || pt == PWR_TYPE_DCDC3
	) {
		reg = TPS6507X_REG_CON_CTRL1;
		msk = CON_CTRL1_LDO2_MASK;
		msk |= CON_CTRL1_DCDC2_MASK;
		msk |= CON_CTRL1_DCDC3_MASK;
		f = on_noff? CON_CTRL1_LDO2_enable: CON_CTRL1_LDO2_disable;
		f |= on_noff? CON_CTRL1_DCDC2_enable: CON_CTRL1_DCDC2_disable;
		f |= on_noff? CON_CTRL1_DCDC3_enable: CON_CTRL1_DCDC3_disable;
	} else {
		return -1;
	}
	v = tps6507x_reg_read(reg);
	v = FIELD_SET(v, msk, f);
	tps6507x_reg_write(reg, v);

	return 0;
}
