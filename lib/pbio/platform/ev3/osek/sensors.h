/**
*  \file sensors.h
*    
*  \brief This header contains macros for the sensor ports of the EV3
* 
*  \author Tobias Schießl
*/

#ifndef SENSORS_H
#define SENSORS_H

/* NXT ports to be API compatible to nxtOSEK */
/**
* \brief NXT sensor port 1
**/ 
#define NXT_PORT_S1     0
/**
* \brief NXT sensor port 2
**/ 
#define NXT_PORT_S2     1
/**
* \brief NXT sensor port 3
**/ 
#define NXT_PORT_S3     2
/**
* \brief NXT sensor port 4
**/ 
#define NXT_PORT_S4     3

/* EV3 ports */
/**
* \brief EV3 sensor port 1
**/ 
#define EV3_PORT_S1     NXT_PORT_S1
/**
* \brief EV3 sensor port 2
**/ 
#define EV3_PORT_S2     NXT_PORT_S2
/**
* \brief EV3 sensor port 3
**/ 
#define EV3_PORT_S3     NXT_PORT_S3
/**
* \brief EV3 sensor port 4
**/ 
#define EV3_PORT_S4     NXT_PORT_S4

#endif // SENSORS_H