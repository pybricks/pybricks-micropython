#ifndef __NXOS_DISPLAY_H__
#define __NXOS_DISPLAY_H__

#include "mytypes.h"

void display_init();

void display_clear();
void display_auto_refresh(bool auto_refresh);
inline void display_refresh();

void display_cursor_set_pos(U8 x, U8 y);
inline void display_end_line();
void display_string(const char *str);
void display_hex(U32 val);
void display_uint(U32 val);

void display_test();

#endif
