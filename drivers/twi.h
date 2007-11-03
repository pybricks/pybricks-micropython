#ifndef __NXOS_TWI_H__
#define __NXOS_TWI_H__

#include "base/mytypes.h"

void nx_twi_init();
void nx_twi_write_async(U32 dev_addr, U8 *data, U32 nBytes);
void nx_twi_read_async(U32 dev_addr, U8 *data, U32 nBytes);
int nx_twi_ready();

#endif
