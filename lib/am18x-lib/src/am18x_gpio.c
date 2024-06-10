// tary, 0:56 2013/4/26
#include "am18x_gpio.h"
#include "am18x_syscfg.h"

#define gcon				GPIOCON
#define reg_and_of(ra, rb)			\
	uint32_t reg, of;			\
	of = reg_offset(bank) + pin;		\
	reg = gcon->BANKS[pair_nr(bank)]. ra##rb


static inline int pair_nr(gpio_bank_t bank) {
	return (bank / 2);
}

static inline int reg_offset(gpio_bank_t bank) {
	if (bank & 0x01UL) {
		return 16;
	}
	return 0;
}

am18x_rt gpio_set_mux(gpio_bank_t bank, gpio_pin_t pin, gpio_dir_t dir) {
	uint32_t mux, pos, val;

	mux = (uint32_t)-1;
	pos = 0;
	val = 8;
	// PINMUX00[7..0] = (GP0[8..15], value = 8)
	// PINMUX01[7..0] = (GP0[0..7], value = 8)
	if (bank == GPIO_BANK0) {
		if (pin < GPIO_PIN_8) {
			mux = 1;
			pos = GPIO_PIN_7 - pin;
		} else {
			mux = 0;
			pos = GPIO_PIN_15 - pin;
		}
	}
	if (bank == GPIO_BANK1) {
	// PINMUX02[0..0] = (GP1[15], value = 8)
		if (pin == GPIO_PIN_15) {
			mux = 2;
			pos = 0;
		} else
	// PINMUX02[6..1] = (GP1[9..14], value = 4)
		if (GPIO_PIN_9 <= pin && pin <= GPIO_PIN_14) {
			mux = 2;
			pos = GPIO_PIN_15 - pin;
			val = 4;
		} else
	// PINMUX03[0..0] = (GP1[8], value = 4)
		if (pin == GPIO_PIN_8) {
			mux = 3;
			pos = 0;
			val = 4;
		} else
	// PINMUX04[7..0] = (GP1[0..7], value = 4)
		if (pin <= GPIO_PIN_7) {
			mux = 4;
			pos = GPIO_PIN_7 - pin;
			val = 4;
		}
	}
	// PINMUX05[7..0] = (GP2[8..15], value = 8)
	// PINMUX06[7..0] = (GP2[0..7], value = 8)
	if (bank == GPIO_BANK2) {
		if (pin < GPIO_PIN_8) {
			mux = 6;
			pos = GPIO_PIN_7 - pin;
		} else {
			mux = 5;
			pos = GPIO_PIN_15 - pin;
		}
	}
	// PINMUX07[7..0] = (GP3[8..15], value = 8)
	// PINMUX08[7..0] = (GP3[0..7], value = 8)
	if (bank == GPIO_BANK3) {
		if (pin < GPIO_PIN_8) {
			mux = 8;
			pos = GPIO_PIN_7 - pin;
		} else {
			mux = 7;
			pos = GPIO_PIN_15 - pin;
		}
	}
	// PINMUX09[7..0] = (GP4[8..15], value = 8)
	// PINMUX10[7..0] = (GP4[0..7], value = 8)
	if (bank == GPIO_BANK4) {
		if (pin < GPIO_PIN_8) {
			mux = 10;
			pos = GPIO_PIN_7 - pin;
		} else {
			mux = 9;
			pos = GPIO_PIN_15 - pin;
		}
	}
	// PINMUX11[7..0] = (GP5[8..15], value = 8)
	// PINMUX12[7..0] = (GP5[0..7], value = 8)
	if (bank == GPIO_BANK5) {
		if (pin < GPIO_PIN_8) {
			mux = 12;
			pos = GPIO_PIN_7 - pin;
		} else {
			mux = 11;
			pos = GPIO_PIN_15 - pin;
		}
	}
	if (bank == GPIO_BANK6) {
	// PINMUX13[7..0] = (GP6[8..15], value = 8)
		if (GPIO_PIN_8 <= pin && pin <= GPIO_PIN_15) {
			mux = 13;
			pos = GPIO_PIN_15 - pin;
		} else
	// PINMUX14[1..0] = (GP6[6..7], value = 8)
		if (GPIO_PIN_6 <= pin && pin <= GPIO_PIN_7) {
			mux = 14;
			pos = GPIO_PIN_7 - pin;
		} else
	// PINMUX16[1..1] = (GP6[5], value = 8)
		if (GPIO_PIN_5 <= pin && pin <= GPIO_PIN_5) {
			mux = 16;
			pos = GPIO_PIN_6 - pin;
		} else
	// PINMUX19[6..2] = (GP6[0..4], value = 8)
		if (pin <= GPIO_PIN_4) {
			mux = 19;
			pos = GPIO_PIN_6 - pin;
		}
	}
	if (bank == GPIO_BANK7) {
	// PINMUX16[7..2] = (GP7[10..15], value = 8)
		if (GPIO_PIN_10 <= pin && pin <= GPIO_PIN_15) {
			mux = 16;
			pos = 17 - pin;
		} else
	// PINMUX17[7..0] = (GP7[2..9], value = 8)
		if (GPIO_PIN_2 <= pin && pin <= GPIO_PIN_9) {
			mux = 17;
			pos = GPIO_PIN_9 - pin;
		} else
	// PINMUX18[1..0] = (GP7[0..1], value = 8)
		if (pin <= GPIO_PIN_1) {
			mux = 18;
			pos = GPIO_PIN_1 - pin;
		}
	}
	if (bank == GPIO_BANK8) {
	// PINMUX18[7..2] = (GP8[10..15], value = 8)
		if (GPIO_PIN_10 <= pin && pin <= GPIO_PIN_15) {
			mux = 18;
			pos = 17 - pin;
		} else
	// PINMUX19[1..0] = (GP8[8..9], value = 8)
		if (GPIO_PIN_8 <= pin && pin <= GPIO_PIN_9) {
			mux = 19;
			pos = GPIO_PIN_9 - pin;
		} else
	// PINMUX02[7..7] = (GP8[7], value = 4)
		if (GPIO_PIN_7 <= pin && pin <= GPIO_PIN_7) {
			mux = 2;
			pos = pin;
			val = 4;
		} else
	// PINMUX03[7..2] = (GP8[1..6], value = 4)
		if (GPIO_PIN_1 <= pin && pin <= GPIO_PIN_6) {
			mux = 3;
			pos = GPIO_PIN_8 - pin;
			val = 4;
		} else
	// PINMUX19[7..7] = (GP8[0], value = 8)
		if (pin <= GPIO_PIN_0) {
			mux = 19;
			pos = GPIO_PIN_7 - pin;
		}
	}

	if (mux == (uint32_t)-1) {
		return AM18X_ERR;
	}

	syscfg_pinmux(mux, pos << 2, val);

	{
	reg_and_of(D,IR);
	gcon->BANKS[pair_nr(bank)].DIR = __field_xset(reg, XXX_GPkPj_MASK(of), 
		(dir == GPIO_DIR_INPUT)? DIR_GPkPj_input(0): DIR_GPkPj_output(0));
	}
	return AM18X_OK;
}

gpio_level_t gpio_get_input1(gpio_bank_t bank, gpio_pin_t pin) {
	reg_and_of(IN,_DATA);

	if (FIELD_GET(reg, XXX_GPkPj_MASK(of)) == IN_DATA_GPkPj_low(of)) {
		return GPIO_LOW;
	}
	return GPIO_HIGH;
}

am18x_rt gpio_set_output1(gpio_bank_t bank, gpio_pin_t pin, gpio_level_t level) {
	reg_and_of(OUT,_DATA);

	gcon->BANKS[pair_nr(bank)].OUT_DATA = __field_xset(reg, XXX_GPkPj_MASK(of), 
		(level == GPIO_LOW)? OUT_DATA_GPkPj_low(0): OUT_DATA_GPkPj_high(0));
	return AM18X_OK;
}

gpio_level_t gpio_get_output1(gpio_bank_t bank, gpio_pin_t pin) {
	reg_and_of(OUT,_DATA);

	if (FIELD_GET(reg, XXX_GPkPj_MASK(of)) == IN_DATA_GPkPj_low(of)) {
		return GPIO_LOW;
	}
	return GPIO_HIGH;	
}

gpio_pins_t gpio_get_inputs(gpio_bank_t bank, gpio_pins_t pins) {
	uint32_t reg, of = reg_offset(bank);

	reg = gcon->BANKS[pair_nr(bank)].IN_DATA;
	reg = FIELD_GET(reg, pins << of);
	return reg >> of;
}

am18x_rt gpio_set_outputs(gpio_bank_t bank, gpio_pins_t pins, gpio_pins_t levels) {
	uint32_t reg, of = reg_offset(bank);

	pins <<= of;
	levels <<= of;
	reg = gcon->BANKS[pair_nr(bank)].OUT_DATA;
	gcon->BANKS[pair_nr(bank)].OUT_DATA = FIELD_SET(reg, pins, levels);
	return AM18X_OK;
}

gpio_pins_t gpio_get_outputs(gpio_bank_t bank, gpio_pins_t pins) {
	uint32_t reg, of = reg_offset(bank);

	reg = gcon->BANKS[pair_nr(bank)].OUT_DATA;
	reg = FIELD_GET(reg, pins << of);
	return reg >> of;
}
