/**
*  \file ev3/i2c.c
*    
*  \brief Function definitions related to I2C communication
* 
*  I2C communication is required in order to interact with the digital sensors.
* 
*  \author Tobias Schießl
*/

/* Include statements */
#include "i2c.h"
#include "soc_AM1808.h"
#include "systick.h"
#include "hw_syscfg0_AM1808.h"
#include "hw_gpio.h"
#include "digi_sensors.h"
#include "evmAM1808.h"
#include <stdio.h>

/* Local function declarations */
void 	doDelay					(int time);
void 	i2c_write_bit			(I2C_PORT port, U8 bit);
U8 		i2c_read_bit			(I2C_PORT port);
U8 		i2c_write_byte			(I2C_PORT port, U8 byte);
U8 		i2c_read_byte			(I2C_PORT port);
void 	i2c_send_start			(I2C_PORT port);
U8 		i2c_send_slave_address	(I2C_PORT port, U8 slave_address, U8 read_write_bit);
void 	i2c_send_stop			(I2C_PORT port);
int 	i2c_available			(int port, U32 address);

/* Macro definitions */
/* Constants */
/**
* \brief The amount of I2C ports on the EV3
**/ 
#define I2C_N_PORTS     4
/**
* \brief The amount of maximum retries before an I2C transaction fails
**/ 
#define I2C_N_RETRIES   3
/* Required pin-multiplexing registers */
/**
* \brief Pin-multiplexing register 0
**/
#define PINMUX0Register             *(unsigned int *)(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0))
/**
* \brief Pin-multiplexing register 1
**/
#define PINMUX1Register             *(unsigned int *)(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1))
/**
* \brief Pin-multiplexing register 2
**/
#define PINMUX2Register             *(unsigned int *)(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(2))
/* Bitmasks to manipulate the pin-multiplexing registers */
/**
* \brief Bitmask to clear the half-byte representing the pin-multiplexing configuration for Port 1 (SDA)
**/
#define MASK_PORT_1_PINMUX0_CLEAR   0xFFFFFFF0
/**
* \brief Bitmask to set the half-byte representing the pin-multiplexing configuration for Port 1 (SDA)
**/
#define MASK_PORT_1_PINMUX0_SET     0x00000008
/**
* \brief Bitmask to clear the half-byte representing the pin-multiplexing configuration for Port 1 (SCL)
**/
#define MASK_PORT_1_PINMUX1_CLEAR   0xFF0FFFFF
/**
* \brief Bitmask to set the half-byte representing the pin-multiplexing configuration for Port 1 (SCL)
**/
#define MASK_PORT_1_PINMUX1_SET     0x00800000
/**
* \brief Bitmask to clear both half-bytes representing the pin-multiplexing configuration for Port 2 (SDA and SCL)
**/
#define MASK_PORT_2_PINMUX0_CLEAR   0xFFFFF00F
/**
* \brief Bitmask to set both half-bytes representing the pin-multiplexing configuration for Port 2 (SDA and SCL)
**/
#define MASK_PORT_2_PINMUX0_SET     0x00000880
/**
* \brief Bitmask to clear the half-byte representing the pin-multiplexing configuration for Port 3 (SDA)
**/
#define MASK_PORT_3_PINMUX0_CLEAR   0xFFFF0FFF
/**
* \brief Bitmask to set the half-byte representing the pin-multiplexing configuration for Port 3 (SDA)
**/
#define MASK_PORT_3_PINMUX0_SET     0x00008000
/**
* \brief Bitmask to clear the half-byte representing the pin-multiplexing configuration for Port 3 (SCL)
**/
#define MASK_PORT_3_PINMUX2_CLEAR   0xFFFFFF0F
/**
* \brief Bitmask to set the half-byte representing the pin-multiplexing configuration for Port 3 (SCL)
**/
#define MASK_PORT_3_PINMUX2_SET     0x00000040
/**
* \brief Bitmask to clear the half-byte representing the pin-multiplexing configuration for Port 4 (SDA)
**/
#define MASK_PORT_4_PINMUX1_CLEAR   0xF0FFFFFF
/**
* \brief Bitmask to set the half-byte representing the pin-multiplexing configuration for Port 4 (SDA)
**/
#define MASK_PORT_4_PINMUX1_SET     0x08000000
/**
* \brief Bitmask to clear the half-byte representing the pin-multiplexing configuration for Port 4 (SCL)
**/
#define MASK_PORT_4_PINMUX2_CLEAR   0xFFFFFF0F
/**
* \brief Bitmask to set the half-byte representing the pin-multiplexing configuration for Port 4 (SCL)
**/
#define MASK_PORT_4_PINMUX2_SET     0x00000008
/* Macros to set a GPIO pin as output */
/**
* \brief Set the SDA pin of the specified port as an output pin
**/
#define SetSDADirectionOutput(port) *((unsigned int *)(SOC_GPIO_0_REGS + GPIO_DIR(port.SDA.gpio_registers))) &= ~port.SDA_SET_CLEAR
/**
* \brief Set the SCL pin of the specified port as an output pin
**/
#define SetSCLDirectionOutput(port) *((unsigned int *)(SOC_GPIO_0_REGS + GPIO_DIR(port.SCL.gpio_registers))) &= ~port.SCL_SET_CLEAR
/**
* \brief Set the SDA and SCL pins of the specified port as output pins
**/
#define SetDirectionOutput(port)    (SetSDADirectionOutput(port)); (SetSCLDirectionOutput(port));
/* Macros to set a GPIO pin as input */
/**
* \brief Set the SDA pin of the specified port as an input pin
**/
#define SetSDADirectionInput(port)  *((unsigned int *)(SOC_GPIO_0_REGS + GPIO_DIR(port.SDA.gpio_registers))) |= port.SDA_SET_CLEAR
/**
* \brief Set the SCL pin of the specified port as an input pin
**/
#define SetSCLDirectionInput(port)  *((unsigned int *)(SOC_GPIO_0_REGS + GPIO_DIR(port.SCL.gpio_registers))) |= port.SCL_SET_CLEAR
/**
* \brief Set the SDA and SCL pins of the specified port as input pins
**/
#define SetDirectionInput           (SetSDADirectionInput(port)); (SetSCLDirectionInput(port));
/* Register required to set SDA/SCL high (needs to be configured as output) */
/**
* \brief Pointer to register required to set the SDA pin of the specified port high
**/
#define SDASetDataRegister(port)    (unsigned int *)(SOC_GPIO_0_REGS + GPIO_SET_DATA(port.SDA.gpio_registers))
/**
* \brief Pointer to register required to set the SCL pin of the specified port high
**/
#define SCLSetDataRegister(port)    (unsigned int *)(SOC_GPIO_0_REGS + GPIO_SET_DATA(port.SCL.gpio_registers))
/* Register required to set SDA/SCL low (needs to be configured as output) */
/**
* \brief Pointer to register required to set the SCL pin of the specified port low
**/
#define SDAClearDataRegister(port)  (unsigned int *)(SOC_GPIO_0_REGS + GPIO_CLR_DATA(port.SDA.gpio_registers))
/**
* \brief Pointer to register required to set the SCL pin of the specified port low
**/
#define SCLClearDataRegister(port)  (unsigned int *)(SOC_GPIO_0_REGS + GPIO_CLR_DATA(port.SCL.gpio_registers))
/* Register required to read SDA/SCL (needs to be configured as input) */
/**
* \brief Pointer to register required to read the SDA pin of the specified port
**/
#define SDAInRegister(port)         (unsigned int *)(SOC_GPIO_0_REGS + GPIO_IN_DATA(port.SDA.gpio_registers))
/**
* \brief Pointer to register required to read the SCL pin of the specified port
**/
#define SCLInRegister(port)         (unsigned int *)(SOC_GPIO_0_REGS + GPIO_IN_DATA(port.SCL.gpio_registers))
/* Macros to set the SDA/SCL pin to low (0) or high (1) (needs to be configured as output) */
/**
* \brief Set the SDA pin of the specified port high (needs to be configured as output)
**/
#define SDA_HIGH(port)              (*(SDASetDataRegister(port))    |= port.SDA_SET_CLEAR);
/**
* \brief Set the SDA pin of the specified port low (needs to be configured as output)
**/
#define SDA_LOW(port)               (*(SDAClearDataRegister(port))  &= port.SDA_SET_CLEAR);
/**
* \brief Set the SCL pin of the specified port high (needs to be configured as output)
**/
#define SCL_HIGH(port)              (*(SCLSetDataRegister(port))    |= port.SCL_SET_CLEAR);
/**
* \brief Set the SCL pin of the specified port low (needs to be configured as output)
**/
#define SCL_LOW(port)               (*(SCLClearDataRegister(port))  &= port.SCL_SET_CLEAR);
/**
* \brief Read the SDA pin of the specified port (needs to be configured as input)
**/
/* Macros to read the value at pin SDA/SCL (needs to be configured as input) */
#define READ_SDA(port)              ((*SDAInRegister(port) & port.SDA_SET_CLEAR) > 0 ? 1 : 0)
/**
* \brief Read the SCL pin of the specified port (needs to be configured as input)
**/
#define READ_SCL(port)              ((*SCLInRegister(port) & port.SCL_SET_CLEAR) > 0 ? 1 : 0)
/* Macro to delay the I2C communication between two clock pulses */
/**
* \brief Delay the communication between two clock pulses
*
* Counting to 85 is the best value to get as close to the original Lego firmware as possible (meassured with an oscilloscope).
* With this value, we have a delay between 100 and 125 ns between two clock pulses.
**/
#define DELAY                       doDelay(85);

/* Local varibale definitions */
/**
* \brief This array stores information about the GPIO pins which represent SDA and SCL lines for the I2C communication
**/ 
I2C_PORT ports_i2c[I2C_N_PORTS] = {
    {{0, 15}, {0, 2}, 0x00008000, 0x00000004},
    {{0, 13}, {0, 14}, 0x00002000, 0x00004000},
    {{0, 14}, {0, 12}, 0x40000000, 0x00001000},
    {{0, 15}, {0, 1}, 0x80000000, 0x00000002}
};

/**
* \brief Delay the communication between two clock pulses by counting from 0 to the specified value
* 
* This approach is necessary since we cannot wait below 1 ms with our systick impplementation and this is far too slow for I2C communication.
*
* \param time - Ther value up to which the counter will be incremented before returning
*
* \return none
**/
void doDelay(int time) {
    for(int j = 0; j < time; ++j);
}

/**
* \brief Disable an I2C port
*
* This function currently has no functionality and therefore it is not necessary to call it.
*
* \param port - The port to disable
*
* \return none
**/
void i2c_disable(int port) {
    // Nothing to do here since we do not use interrupts (there is no communication since no transaction is started)
}

/**
* \brief Enable an I2C port
*
* Enabling a port means configuring the corresponding pin-multiplexing registers and setting the pins as output pins with a high signal.
*
* \param port - The port to disable
*
* \return none
**/
void i2c_enable(int port) {
    SysCfgRegistersUnlock();
    // Set PINMUX register
    switch (port) {
        case NXT_PORT_S1: {
            PINMUX0Register = (PINMUX0Register & MASK_PORT_1_PINMUX0_CLEAR) | MASK_PORT_1_PINMUX0_SET;
            PINMUX1Register = (PINMUX1Register & MASK_PORT_1_PINMUX1_CLEAR) | MASK_PORT_1_PINMUX1_SET;
            break;
        }
        case NXT_PORT_S2: {
            PINMUX0Register = (PINMUX0Register & MASK_PORT_2_PINMUX0_CLEAR) | MASK_PORT_2_PINMUX0_SET;
            break;
        }
        case NXT_PORT_S3: {
            PINMUX0Register = (PINMUX0Register & MASK_PORT_3_PINMUX0_CLEAR) | MASK_PORT_3_PINMUX0_SET;
            PINMUX2Register = (PINMUX2Register & MASK_PORT_3_PINMUX2_CLEAR) | MASK_PORT_3_PINMUX2_SET;
            break;
        }
        case NXT_PORT_S4: {
            PINMUX1Register = (PINMUX1Register & MASK_PORT_4_PINMUX1_CLEAR) | MASK_PORT_4_PINMUX1_SET;
            PINMUX2Register = (PINMUX2Register & MASK_PORT_4_PINMUX2_CLEAR) | MASK_PORT_4_PINMUX2_SET;
            break;
        }
        default: 
            printf("Unknown port number: %i\n ", port); 
            return;
    }
    // Set direction to output
    I2C_PORT p = ports_i2c[port];
    SetDirectionOutput(p);
    SysCfgRegistersLock();
}

/**
* \brief Initialize the I2C module
*
* This function currently has no functionality and therefore it is not necessary to call it.
*
* \return none
**/
void i2c_init(void) {
    // Nothing to do here (if we would use interrupts, we could initialize the AINTC here)
}


/**
* \brief Check whether an I2C port is currently busy or not
*
* Since our I2C communication is currently not based on interrupts (as it is in the NXT version of the leJOS driver) a port is never busy. Therefore this function will always return 0.
*
* \param port - The port to check
*
* \return Always 0 since our port can never be busy (no transaction will be pending)
**/
int i2c_busy(int port) {
    return 0;
}

/**
* \brief Send a bit by setting the SDA pin high or low
*
* Only the last bit of the given byte will be used (LSB).
* SCL should be low before calling this function and will be low again after returning from it.
*
* \param port - The port in use
* \param bit - The bit to write (only the LSB of the byte will be considered)
*
* \return none
**/
void i2c_write_bit(I2C_PORT port, U8 bit) {
    bit &= 0x01; // Only use LSB
    if (bit == 1) {
        SDA_HIGH(port); // Not while SCL is 1
    }
    else {
        SDA_LOW(port); // Not while SCL is 1
    }
    DELAY;
    SCL_HIGH(port);
    DELAY;
    SCL_LOW(port);
}

/**
* \brief Read a bit by reading the signal on SDA
*
* Only the last bit of the returned byte should be considered (LSB). All other bits will be 0.
* This function will set SDA as an input pin in order to give the control to the slave device (sensor). The pin will be reset to output before returning from this function.
* SCL should be low before calling this function and will be low again after returning from it.
*
* \param port - The port in use
*
* \return The bit red (LSB of the returned byte)
**/
U8 i2c_read_bit(I2C_PORT port) {
    SysCfgRegistersUnlock();
    SetSDADirectionInput(port);    
    DELAY; // let slave drive SDA
    SCL_HIGH(port);
    U8 bit = READ_SDA(port);
    DELAY;
    SCL_LOW(port);
    SetSDADirectionOutput(port);
    SysCfgRegistersLock();
    return bit;
}

/**
* \brief Send a byte
*
* The byte will be written by sending it bit by bit. Therefore i2c_write_bit will be used.
* SCL should be low before calling this function and will be low again after returning from it.
*
* \param port - The port in use
* \param byte - The byte to send
*
* \return The ACK bit received from the slave (0 for ACK and 1 for no ACK)
**/
U8 i2c_write_byte(I2C_PORT port, U8 byte) {
    // Write the given byte bitweise by calling i2c_write_bit
    for (int i = 0; i < 8; ++i) {
        U8 bit = byte >> (7 - i);
        i2c_write_bit(port, bit);
    }
    // Read ACK
    U8 ack = i2c_read_bit(port);
    return ack;
}

/**
* \brief Read a byte
*
* The byte will be red by reading it bit by bit. Therefore i2c_read_bit will be used.
* Before returning, this function will send an ACK bit to the slave device (sensor).
* SCL should be low before calling this function and will be low again after returning from it.
*
* \param port - The port in use
*
* \return The byte red
**/
U8 i2c_read_byte(I2C_PORT port) {
    // Read the data bitweise by calling i2c_read_bit
    U8 byte = 0;
    for (int i = 0; i < 8; ++i) {
        U8 bit = i2c_read_bit(port);
        byte |= (bit << (7 - i));
    }
    // Send ACK: 0 == OKAY, 1 == ERROR
    i2c_write_bit(port, 0);
    return byte;
}

/**
* \brief Send an I2C start signal
*
* A start in an I2C transaction means setting SDA to 0 while SCL is 1.
* After calling this function, SDA and SCL will both be low.
*
* \param port - The port in use
*
* \return none
**/
void i2c_send_start(I2C_PORT port) {
    // Check if SCL and SDA are 0 - if so, we sent a repeated start and need to set SCL and SDA high at first
    if (READ_SCL(port) == 0) {
        SDA_HIGH(port);
        DELAY;
        SCL_HIGH(port);
        DELAY;
    }
    // Set SDA to 0 while SCL is 1
    SDA_LOW(port); // while SCL is 1 --> okay at this place
    DELAY;
    SCL_LOW(port);
    DELAY;
}

/**
* \brief Send the I2C slave address along with the read/write bit
*
* This is the first byte that has to be sent in an I2C transaction after sending the start signal.
* The slave address is a 7 bit address which gets shifted by 1 to the left inside this function. The last bit of the byte will be 1 if we want to write to the slave device (sensor), 0 if we want to read from it.
* SCL should be low before calling this function and will be low again after returning from it.
*
* \param port - The port in use
* \param slave_address - The slave address to use (unshifted)
* \param read_write_bit - The read/write bit (1 if we want to write to the slave device, 0 if we want to read from it)
*
* \return The ACK bit received from the slave (0 for ACK and 1 for no ACK)
**/
U8 i2c_send_slave_address(I2C_PORT port, U8 slave_address, U8 read_write_bit) {
    // Shift slave address by one bit to the left and append the read write bit on the right side
    // Then send the new byte
    U8 byteToSend = (slave_address << 1) | (read_write_bit & 0x01);
    U8 ack = i2c_write_byte(port, byteToSend);
    return ack;
}

/**
* \brief Send an I2C stop signal
*
* A stop in an I2C transaction means setting SDA to 1 while SCL is 1.
* After calling this function, SDA and SCL will both be high.
*
* \param port - The port in use
*
* \return none
**/
void i2c_send_stop(I2C_PORT port) {
    // Set SDA to 1 while SCL is 1
    SCL_LOW(port);
    DELAY;
    SDA_LOW(port); // not while SCL is 1
    DELAY;
    SCL_HIGH(port);
    DELAY;
    SDA_HIGH(port); // while SCL is 1 --> okay at this place
    DELAY;
}

/**
* \brief Check if an I2C device with the given address is available at the specified with port
*
* This function makes use of the internal register layout which is equal among all Lego/HiTechnic I2C sensors. On internal address 0x00, the product version if the sensor is stored.
* If we read this value and receive a 0, we can assume that no sensor is available since a product version of 0 makes no sense. So far this is the only way to figure out if a sensor is connected to the port.
* Since the SDA signal will default to 0 when configured as input, the ACK bit is not enough to check that. It would always be 0 and therefore we would assume that a device is ready.
*
* \param port - The port in use
* \param address - The I2C slave address of the slave device
*
* \return 0 if no device is available, 1 otherwise
**/
int i2c_available(int port, U32 address) {
    I2C_PORT p = ports_i2c[port];
	
	// Send stop in order for transaction to work as expected - otherwise, the Lego ultrasonic sensor will not answer
    i2c_send_stop(p);
    systick_wait_ms(100);
    
    i2c_send_start(p);
    
    U8 addressU8 = (U8) address;
    U8 ack = i2c_send_slave_address(p, addressU8, 0);
    if (ack == 1)
        return 0;
    
    U8 registerAddress = 0x00; // address of register for product version
    ack = i2c_write_byte(p, registerAddress);
    if (ack == 1)
        return 0;
    
    // Since the Lego ultrasonic sensor does not follow standard I2C protocol, we need to send an additional stop and 1 additional clock pulse before the restart
    i2c_send_stop(p);
    DELAY;
    SCL_HIGH(p);
    DELAY;
    SCL_LOW(p);
    systick_wait_ms(100);
    
    i2c_send_start(p);
    
    ack = i2c_send_slave_address(p, addressU8, 1);
    if (ack == 1)
        return 0;
    
    U8 byte = i2c_read_byte(p);
    
    i2c_send_stop(p);
    
    if (byte == 0)
        // There is no slave device with a version of 0
        return 0;
    else
        // Something was red so a device must be available
        return 1;
}

/**
* \brief Start a new I2C transaction AND wait until it's finished
*
* In contrast to the name of the function, this will not only start the I2C transaction. Instead, the whole transaction is processed before returning from this function.
* The name remains in order to be API-compatible with the NXT version of the leJOS driver. In this driver, the I2C communication was controlled using interrupts.
* Note: According to the I2C protocol, a write is represented by a 0 and not by a 1 as in this function call. This behaviour is intended an also to keep API-compatibility to the NXT version of the driver.
*
* \param port - The port to use (0 to 3)
* \param address - The I2C slave address of the slave device
* \param internal_address - Internal register address of slave device to read from or write to
* \param n_internal_address_bytes - Size of the internal register address in bytes
* \param data - Buffer to store received data in or the data to send
* \param nbytes - Amount of bytes to receive or send, i.e. the length of the data array
* \param write - 1 if we want to write to the slave, 0 if we want to read from it
*
* \return 0 if the transaction was successful, otherwise the number of the byte for which no ACK was received (beginning with 1)
**/
int i2c_start_transaction(int port, U32 address, int internal_address, int n_internal_address_bytes, U8 *data, U32 nbytes, int write) {
    int j = 0; // Retry counter
    while (1) {
        ++j;
        
        /* This could be used to determine if a sensor is available or not */
		/* For performance reasons we do not use it right now and assume that the developper knowns which port is connected to a sensor and which one isn't */
        // if (!i2c_available(port, address)) return -1;
        
        I2C_PORT p = ports_i2c[port];
        
        // Send stop in order for transaction to work as expected - otherwise, the Lego ultrasonic sensor will not answer
        i2c_send_stop(p);
        systick_wait_ms(10);
        
        // Send Start
        i2c_send_start(p);
        
        // Send slave address with read-write-bit set to 0 (LSB) which means write in I2C protocol
        U8 address_U8 = (U8) address;
        U8 ack = i2c_send_slave_address(p, address_U8, 0);
        if (ack == 1) {
            // Retries are required since the HiTechnic sensors only answer every second time (TODO: find out why)
            if (j == I2C_N_RETRIES) 
                return 1; 
            else 
                continue;
        }
        
        // Send register address to read from / write to
        U8 register_address = (U8) internal_address;
        ack = i2c_write_byte(p, register_address);
        if (ack == 1) {
            if (j == I2C_N_RETRIES) 
                return 2; 
            else 
                continue;
        }
        
        if (write == 0) { // Read data from slave 
            // Since the Lego ultrasonic sensor does not follow standard I2C protocol, we need to send an Additional stop and 1 Additional clock pulse before the restart
            i2c_send_stop(p);
            DELAY;
            SCL_HIGH(p);
            DELAY;
            SCL_LOW(p);
            systick_wait_ms(10);
            
            // Send another Start (repeated start)
            i2c_send_start(p);
            
            // Send slave address with read-write-bit set to 1 (LSB) which means read in I2C protocol
            ack = i2c_send_slave_address(p, address_U8, 1);
            if (ack == 1) {
                if (j == I2C_N_RETRIES) 
                    return 3; 
                else 
                    continue;
            }
            
            for (int i = 0; i < nbytes; ++i) {
                // Read 1 Byte
                U8 byte = i2c_read_byte(p);
                *(data + i) = byte;
            }
        }
        else { // Write to the slave
            for (int i = 0; i < nbytes; ++i) {
                // Write one byte
                ack = i2c_write_byte(p, *(data + i));
                if (ack == 1) {
                    if (j == I2C_N_RETRIES) 
                        return 3 + i; 
                    else 
                        continue;
                }
            }
        }
        
        // Send Stop
        i2c_send_stop(p);
        
        return 0;
    }
}
