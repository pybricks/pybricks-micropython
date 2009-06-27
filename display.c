/* Copyright (c) 2008 the NxOS developers
 *
 * See AUTHORS for a full list of the developers.
 *
 * Redistribution of this file is permitted under
 * the terms of the GNU Public License (GPL) version 2.
 */

#include "base/types.h"
#include "base/interrupts.h"
#include "base/util.h"
#include "base/assert.h"
#include "base/drivers/systick.h"
#include "base/drivers/aic.h"
#include "base/drivers/_lcd.h"

#include "base/_display.h"

/* A simple 8x5 font. This is in a separate file because the embedded
 * font is converted from a .png at compile time.
 */
#include "_font.h"

static struct {
  /* The display buffer, which is mirrored to the LCD controller's RAM. */
  U8 buffer[LCD_HEIGHT][LCD_WIDTH];

  /* Whether the display is automatically refreshed after every call
   * to display functions. */
  bool auto_refresh;

  /* The position of the text cursor. This is used for easy displaying
   * of text in a console-like manner.
   */
  struct {
    U8 x;
    U8 y;
    bool ignore_lf; /* If the display just wrapped from the right side
                       of the screen, ignore an LF immediately
                       after. */
  } cursor;
} display;


static inline void dirty_display(void) {
  if (display.auto_refresh)
    nx__lcd_dirty_display();
}


/* Clear the display. */
void nx_display_clear(void) {
  memset(&display.buffer[0][0], 0, sizeof(display.buffer));
  nx_display_cursor_set_pos(0, 0);
  dirty_display();
}


/* Enable or disable auto-refresh. */
void nx_display_auto_refresh(bool auto_refresh) {
  display.auto_refresh = auto_refresh;
  dirty_display();
}


/* Explicitely refresh the display. You only need to use this when
 * auto-refresh is disabled.
 */
inline void nx_display_refresh(void) {
  nx__lcd_dirty_display();
}


/*
 * Text display functions.
 */
static inline bool is_on_screen(U8 x, U8 y) {
  if (x < NX__DISPLAY_WIDTH_CELLS &&
      y < NX__DISPLAY_HEIGHT_CELLS)
    return TRUE;
  else
    return FALSE;
}

static inline const U8 *char_to_font(const char c) {
  if (c >= NX__FONT_START)
    return nx__font_data[c - NX__FONT_START];
  else
    return nx__font_data[0]; /* Unprintable characters become spaces. */
}

static inline void update_cursor(bool inc_y) {
  if (!inc_y) {
    display.cursor.x++;

    if (display.cursor.x >= LCD_WIDTH / NX__CELL_WIDTH) {
      display.cursor.x = 0;
      display.cursor.y++;
      display.cursor.ignore_lf = TRUE;
    } else {
      display.cursor.ignore_lf = FALSE;
    }
  } else if (display.cursor.ignore_lf) {
    display.cursor.ignore_lf = FALSE;
  } else {
    display.cursor.x = 0;
    display.cursor.y++;
  }

  if (display.cursor.y >= LCD_HEIGHT)
    display.cursor.y = 0;
}

void nx_display_cursor_set_pos(U8 x, U8 y) {
  NX_ASSERT(is_on_screen(x, y));
  display.cursor.x = x;
  display.cursor.y = y;
}

inline void nx_display_end_line(void) {
  update_cursor(TRUE);
}

void nx_display_string(const char *str) {
  while (*str != '\0') {
    if (*str == '\n')
      update_cursor(TRUE);
    else {
      int x_offset = display.cursor.x * NX__CELL_WIDTH;
      memcpy(&display.buffer[display.cursor.y][x_offset],
             char_to_font(*str), NX__FONT_WIDTH);
      update_cursor(FALSE);
    }
    str++;
  }
  dirty_display();
}

void nx_display_hex(U32 val) {
  const char hex[16] = "0123456789ABCDEF";
  char buf[9];
  char *ptr = &buf[8];

  if (val == 0) {
    ptr--;
    *ptr = hex[0];
  } else {
    while (val != 0) {
      ptr--;
      *ptr = hex[val & 0xF];
      val >>= 4;
    }
  }

  buf[8] = '\0';

  nx_display_string(ptr);
  dirty_display();
}

void nx_display_uint(U32 val) {
  char buf[11];
  char *ptr = &buf[10];

  if (val == 0) {
    ptr--;
    *ptr = '0';
  } else {

    while (val > 0) {
      ptr--;
      *ptr = val % 10 + '0';
      val /= 10;
    }

  }

  buf[10] = '\0';

  nx_display_string(ptr);
  dirty_display();
}

void nx_display_int(S32 val) {
  if( val < 0 ) {
    nx_display_string("-");
    val = -val;
  }
  nx_display_uint(val);
}

/*
 * Display initialization.
 */
void nx__display_init(void) {
  display.auto_refresh = FALSE;
  nx_display_clear();
  display.cursor.x = 0;
  display.cursor.y = 0;
  display.cursor.ignore_lf = FALSE;
  nx__lcd_set_display(&display.buffer[0][0]);
  display.auto_refresh = TRUE;
  dirty_display();
}
