/* GUI library for NxOS.
 */

#include "base/types.h"
#include "base/util.h"
#include "base/assert.h"
#include "base/display.h"
#include "base/drivers/avr.h"
#include "base/drivers/systick.h"
#include "base/lib/gui/gui.h"

U8 nx_gui_text_menu(gui_text_menu_t menu) {
  U8 current = 0, count = 0, i;
  bool done = FALSE;

  NX_ASSERT(strlen(menu.title) > 0);
  
  for (i=0; menu.entries[i]; count++, i++);
  NX_ASSERT(count > 0);

  if (menu.default_entry < count)
    current = menu.default_entry;

  do {
    nx_avr_button_t button;

    nx_display_clear();
    nx_display_string(menu.title);
    nx_display_end_line();
    nx_display_end_line();

    for (i=0; i<count; i++) {
      nx_display_string(" ");
      nx_display_uint(i+1);

      if (current == i)
        nx_display_string(menu.active_mark);
      else
        nx_display_string(". ");

      nx_display_string(menu.entries[i]); 
      nx_display_end_line();
    }

    nx_systick_wait_ms(GUI_EVENT_THROTTLE);
    while ((button = nx_avr_get_button()) == BUTTON_NONE);

    switch (button) {
      case BUTTON_LEFT:
        if (current > 0)
          current--;
        break;
      case BUTTON_RIGHT:
        if (current < count-1)
          current++;
        break;
      case BUTTON_OK:
        done = TRUE;
        break;
      default:
        break;
    }
  } while (!done); 


  nx_systick_wait_ms(GUI_EVENT_AVOID_REPEAT);
  return current;
}

