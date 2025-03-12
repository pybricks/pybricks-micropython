/**
*  \file ev3/i2c.h
*    
*  \brief Function declarations related to I2C communication
* 
*  I2C communication is required in order to interact with the digital sensors.
* 
*  \author Tobias Schießl
*/

#ifndef I2C_H
#define I2C_H

/* Include statements */
#include "mytypes.h"

/* Struct definitions */
/**
 * \brief This struct represents a GPIO pin used for I2C communication
 * 
 * Every pin is represented by a pin number on a GPIO bank (0 to 15) and one GPIO register to control that pin.
 * Note: 2 GPIO banks always share one GPIO register, therefore bank 0 and 1 relate to register 0, bank 2 and 3 relate to register 1 and so on.
 */
typedef struct I2C_PIN {
    U8 gpio_registers;			///< The GPIO register to control this pin
    U8 pin;						///< The number of the GPIO pin on its bank
} I2C_PIN;

/**
 * \brief This struct represents an I2C port
 * 
 * Every port is represented by two GPIO pins (SDA for data and SCL for clock). This struct also contains the corresponding bitmasks in order to manipulate said GPIO pins.
 */
typedef struct I2C_PORT {
    I2C_PIN SDA;				///< GPIO pin SDA (data)
    I2C_PIN SCL;				///< GPIO pin SCL (clock)
    unsigned int SDA_SET_CLEAR;	///< Bitmask to set/clear the SDA pin
    unsigned int SCL_SET_CLEAR;	///< Bitmask to set/clear the SCL pin
} I2C_PORT;

/* I2C interface */
void 	i2c_disable				(int port);
void 	i2c_enable				(int port);
void 	i2c_init				(void);
int 	i2c_busy				(int port);
int 	i2c_start_transaction	(int port, U32 address, int internal_address, int n_internal_address_bytes, U8 *data, U32 nbytes,int write);

#endif // I2C_H