// tary, 0:31 2013/4/26

#ifndef __AM18X_GPIO_H__
#define __AM18X_GPIO_H__

#include "am18x_map.h"

typedef enum {
	GPIO_LOW = 0,
	GPIO_HIGH = 1,
} gpio_level_t;

typedef enum {
	GPIO_BANK0,
	GPIO_BANK1,
	GPIO_BANK2,
	GPIO_BANK3,
	GPIO_BANK4,
	GPIO_BANK5,
	GPIO_BANK6,
	GPIO_BANK7,
	GPIO_BANK8,
} gpio_bank_t;

typedef enum {
	GPIO_PIN_0,
	GPIO_PIN_1,
	GPIO_PIN_2,
	GPIO_PIN_3,
	GPIO_PIN_4,
	GPIO_PIN_5,
	GPIO_PIN_6,
	GPIO_PIN_7,
	GPIO_PIN_8,
	GPIO_PIN_9,
	GPIO_PIN_10,
	GPIO_PIN_11,
	GPIO_PIN_12,
	GPIO_PIN_13,
	GPIO_PIN_14,
	GPIO_PIN_15,
} gpio_pin_t;

typedef enum {
	GPIO_PINS_0 = BIT(0),
	GPIO_PINS_1 = BIT(1),
	GPIO_PINS_2 = BIT(2),
	GPIO_PINS_3 = BIT(3),
	GPIO_PINS_4 = BIT(4),
	GPIO_PINS_5 = BIT(5),
	GPIO_PINS_6 = BIT(6),
	GPIO_PINS_7 = BIT(7),
	GPIO_PINS_8 = BIT(8),
	GPIO_PINS_9 = BIT(9),
	GPIO_PINS_10 = BIT(10),
	GPIO_PINS_11 = BIT(11),
	GPIO_PINS_12 = BIT(12),
	GPIO_PINS_13 = BIT(13),
	GPIO_PINS_14 = BIT(14),
	GPIO_PINS_15 = BIT(15),
	GPIO_PINS_ALL = BIT(16) - 1,
} gpio_pins_cell_t;

typedef enum {
	GPIO_DIR_INPUT,
	GPIO_DIR_OUTPUT,
} gpio_dir_t;

typedef uint32_t			gpio_pins_t;

am18x_rt gpio_set_mux(gpio_bank_t bank, gpio_pin_t pin, gpio_dir_t dir);

gpio_level_t gpio_get_input1(gpio_bank_t bank, gpio_pin_t pin);
am18x_rt gpio_set_output1(gpio_bank_t bank, gpio_pin_t pin, gpio_level_t level);
/* get the level last output */
gpio_level_t gpio_get_output1(gpio_bank_t bank, gpio_pin_t pin);

/* 
  bit set == level high
  bit reset == level low
*/
gpio_pins_t gpio_get_inputs(gpio_bank_t bank, gpio_pins_t pins);
am18x_rt gpio_set_outputs(gpio_bank_t bank, gpio_pins_t pins, gpio_pins_t levels);
/* get levels last output */
gpio_pins_t gpio_get_outputs(gpio_bank_t bank, gpio_pins_t pins);

#endif//__AM18X_GPIO_H__
