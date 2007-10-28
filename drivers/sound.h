#ifndef __NXOS_SOUND_H__
#define __NXOS_SOUND_H__

#include "mytypes.h"

void sound_init();
void sound_freq_async(U32 freq, U32 ms);
void sound_freq(U32 freq, U32 ms);

#endif
