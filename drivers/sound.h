#ifndef __NXOS_SOUND_H__
#define __NXOS_SOUND_H__

#include "base/mytypes.h"

void nx_sound_init();
void nx_sound_freq_async(U32 freq, U32 ms);
void nx_sound_freq(U32 freq, U32 ms);

#endif
