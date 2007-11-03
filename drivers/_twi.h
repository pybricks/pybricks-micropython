#ifndef __NXOS_BASE_DRIVERS__TWI_H__
#define __NXOS_BASE_DRIVERS__TWI_H__

#include "base/types.h"

void nx__twi_init();
void nx__twi_write_async(U32 dev_addr, U8 *data, U32 nBytes);
void nx__twi_read_async(U32 dev_addr, U8 *data, U32 nBytes);
int nx__twi_ready();

#endif
