#ifndef __NXOS_DISPLAY_H__
#define __NXOS_DISPLAY_H__

#include "base/types.h"

void nx_display_clear();
void nx_display_auto_refresh(bool auto_refresh);
inline void nx_display_refresh();

void nx_display_cursor_set_pos(U8 x, U8 y);
inline void nx_display_end_line();
void nx_display_string(const char *str);
void nx_display_hex(U32 val);
void nx_display_uint(U32 val);

#endif
