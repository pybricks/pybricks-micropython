// tary, 23:10 2013/3/25
#ifndef __TCA6416_H__
#define __TCA6416_H__

#include "am18x_lib.h"

// low byte as Port 0, high byte as Port 1
// Port0.Pin1 as bit 1, Port1.Pin4 as bit 12, etc

// bit set as input, bit clear as output
// -1 as error, 0 as success
int tca6416_conf(uint16_t dir);
// bit set as high level, bit clear as low level
// -1 as error, the port value otherwise
int tca6416_read(void);
// -1 as error, 0 as success
int tca6416_write(uint16_t val);

#endif//__TCA6416_H__
