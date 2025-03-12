/**
*  \file adc.h
*    
*  \brief This header contains function declarations to talk to the ADC
* 
*  \author ev3ninja
*/

#pragma once

unsigned short 	adc_get		(unsigned short channel);
void 			adc_init	(void);