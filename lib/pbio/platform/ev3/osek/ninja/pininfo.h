/**
*  \file pininfo.h
*    
*  \brief This header defines the struct that represents a GPIO pin in the ev3ninja driver and makes the corresponding variable in pininfo.c accessible by other files
* 
*  \author ev3ninja
*/

#pragma once

/**
* \brief This struct represents 1 GPIO pin
*/
typedef struct pin_info {
	unsigned int muxreg;		///< The PINMUX register for this pin (ranging from 0 to 19)
	unsigned int muxreg_mask;	///< The bitmask to clear the half-byte in the PINMUX register for this pin
	unsigned int muxreg_mode;   ///< The bitmask to set the half-byte in the PINMUX register for this pin to the desired value
} pin_info;

// Defined in pininfo.c
extern pin_info 		pininfo[]; 
extern unsigned int 	pininfo_size;