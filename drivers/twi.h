#ifndef __NXOS_TWI_H__
#define __NXOS_TWI_H__

#include "mytypes.h"

void twi_init();
void twi_write_async(U32 dev_addr, U8 *data, U32 nBytes);
void twi_read_async(U32 dev_addr, U8 *data, U32 nBytes);
int twi_ready();

#endif
