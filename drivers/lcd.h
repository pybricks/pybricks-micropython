#ifndef __NXOS_LCD_H__
#define __NXOS_LCD_H__

/* Width and height of the LCD display. */
#define LCD_WIDTH 100 /* pixels */
#define LCD_HEIGHT 8 /* bytes, so 64 pixels. */

void nx_lcd_init();
void nx_lcd_fast_update();
void nx_lcd_set_display(U8 *display_buffer);
inline void nx_lcd_dirty_display();
void nx_lcd_shutdown();
void nx_lcd_test();

#endif
