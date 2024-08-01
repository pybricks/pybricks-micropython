// tary, 23:04 2013/3/25
#include "am18x_lib.h"
#include "tca6416.h"
#include "auxlib.h"
#include "i2c_inf.h"

// 1015526A_AM1808_Baseboard_Schematic.pdf
// Page 14, IO EXPANDER
#define TCA6416_BUS			I2C0

// tca6416.pdf
// Device Address
#define TCA6416_ADDR			0x21	// 0x20

#define TCA6416_SPEED			350000	// 400K

// Control Register and Command Byte
// Once a new command has been sent, the register that
// was addressed continues to be accessed by reads until
// a new command byte has been sent.
typedef enum {
	// reflect the incomming logic of the pins,
	// regardless of whether the pin is defined as
	// an input or an output by the Configuration register
	INPUT_PORT_0,
	INPUT_PORT_1,
	// value that is in the flip-flop controlling
	// the output selection, NOT the actual pin value
	OUTPUT_PORT_0,
	OUTPUT_PORT_1,
	// allow polarity inversion of pins defined as inputs
	// by the Configuration register.
	POLARITY_INVERSION_PORT_0,
	POLARITY_INVERSION_PORT_1,
	// configure the direction of the I/O pins.
	// If a bit in these register is set to 1,
	// the corresponding port pin is enabled as
	// an input with a high-impedance output driver.
	CONFIGURATION_PORT_0,
	CONFIGURATION_PORT_1,
} TCA6416_reg_t;

static inline int tca6416_reg_write(uint8_t reg, uint16_t val) {
	uint8_t bytes[3];

	bytes[0] = reg;
	bytes[1] = FIELD_XGET(val, 0x00FFUL);
	bytes[2] = FIELD_XGET(val, 0xFF00UL);

	i2c_write(TCA6416_BUS, TCA6416_ADDR, bytes, sizeof bytes);

	return 0;
}

static inline int tca6416_reg_read(uint8_t reg) {
	uint8_t bytes[2];
	uint16_t val = 0;

	bytes[0] = reg;
	i2c_write(TCA6416_BUS, TCA6416_ADDR, bytes, 1);
	i2c_read(TCA6416_BUS, TCA6416_ADDR, bytes, sizeof bytes);

	val = FIELD_XSET(val, 0x00FFUL, bytes[0]);
	val = FIELD_XSET(val, 0xFF00UL, bytes[1]);
	return val;
}

int tca6416_conf(uint16_t dir) {
	int i;

	i2c_init(TCA6416_BUS, TCA6416_SPEED);

	for (i = 0; i < 8 ; i += 2) {
		printk("[0x%.2X] = 0x%.4X\n", i, tca6416_reg_read(i));
	}
	tca6416_reg_write(POLARITY_INVERSION_PORT_0, 0x0000UL);
	tca6416_reg_write(CONFIGURATION_PORT_0, dir);
	return 0;
}

int tca6416_read(void) {
	uint16_t v;

	v = tca6416_reg_read(INPUT_PORT_0);
	return v;
}

int tca6416_write(uint16_t val) {
	tca6416_reg_write(OUTPUT_PORT_0, val);
	return 0;
}
