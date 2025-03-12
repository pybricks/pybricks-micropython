/**
*  \file ninja/spi.h
*    
*  \brief This header declares function required to interact with the SPI0 controller of the SoC which is connected to the ADC.
* 
*  \author ev3ninja
*/

#pragma once

/* SPI interface */
unsigned short 	spi_update	(unsigned short data);
void 			spi_init	(void);