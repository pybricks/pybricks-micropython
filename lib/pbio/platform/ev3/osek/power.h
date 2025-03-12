/**
*  \file power.h
*    
*  \brief This header contains function declarations for the power management on the EV3.
* 
*  \author Tobias Schießl
*/

#ifndef POWER_H
#define POWER_H

/* Include statements */
#include "mytypes.h"

void 	power_off			(void);
U16 	get_battery_voltage	(void);
U16 	get_battery_vurrent	(void);

#endif // POWER_H