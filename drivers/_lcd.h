#ifndef __NXOS_BASE_DRIVERS__LCD_H__
#define __NXOS_BASE_DRIVERS__LCD_H__

/* Width and height of the LCD display. */
#define LCD_WIDTH 100 /* pixels */
#define LCD_HEIGHT 8 /* bytes, so 64 pixels. */

void nx__lcd_init();
void nx__lcd_fast_update();
void nx__lcd_set_display(U8 *display_buffer);
inline void nx__lcd_dirty_display();
void nx__lcd_shutdown();

#endif
