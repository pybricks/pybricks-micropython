#ifndef __NXOS_LCD_H__
#define __NXOS_LCD_H__

/* Width and height of the LCD display. */
#define LCD_WIDTH 100 /* pixels */
#define LCD_HEIGHT 8 /* bytes, so 64 pixels. */

void lcd_init();
void lcd_1kHz_update();
void lcd_set_display(U8 *display_buffer);
inline void lcd_dirty_display();
void lcd_shutdown();
void lcd_test();

#endif
