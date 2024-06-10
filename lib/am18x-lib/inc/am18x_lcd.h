// tary, 17:57 2013/12/8

#ifndef __AM18X_LCD_H__
#define __AM18X_LCD_H__

#include "am18x_map.h"

typedef enum {
	LCD_BPP_1 = 1,
	LCD_BPP_2 = 2,
	LCD_BPP_4 = 4,
	LCD_BPP_8 = 8,
	LCD_BPP_16 = 16,
} lcd_bpp_t;

typedef enum {
	LCD_CFLAG_BIAS_HIGH = 0,
	LCD_CFLAG_BIAS_LOW = BIT(0),
	LCD_CFLAG_PIXEL_RISING = 0,
	LCD_CFLAG_PIXEL_FALLING = BIT(1),
	LCD_CFLAG_HSYNC_HIGH = 0,
	LCD_CFLAG_HSYNC_LOW = BIT(2),
	LCD_CFLAG_VSYNC_HIGH = 0,
	LCD_CFLAG_VSYNC_LOW = BIT(3),
} lcd_cflag_t;

typedef enum {
	LCD_HVSYNC_PCLK,
	LCD_HVSYNC_RISING,
	LCD_HVSYNC_FALLING,
} lcd_hvsync_t;

typedef struct {
	uint32_t	pclk;
	uint32_t	width, height;
	uint16_t	hfp, hsw, hbp;
	uint16_t	vfp, vsw, vbp;
	uint8_t		bpp;
	uint8_t		hvsync;
	uint16_t	cflag;
	uint32_t	fb0_base;
	uint32_t	fb1_base;
} lcd_conf_t;

typedef enum {
	LCD_CMD_RASTER_EN,
	LCD_CMD_RASTER_DIS,
	LCD_CMD_PALETTE,
	LCD_CMD_DATA,
	LCD_CMD_FB_SET,
} lcd_cmd_t;

typedef enum {
	LCD_INTR_AC,
	LCD_INTR_DONE,
	LCD_INTR_PL,
	LCD_INTR_SL,
	LCD_INTR_FUF,
	LCD_INTR_EOF,
	// only for lcd_intr_state(), lcd_intr_clear()
	LCD_INTR_EOF1,
	// only for lcd_intr_clear()
	LCD_INTR_ALL,
} lcd_intr_t;

am18x_rt lcd_conf(LCD_con_t* lcon, const lcd_conf_t* conf);
am18x_rt lcd_cmd(LCD_con_t* lcon, uint32_t cmd, uint32_t arg);
am18x_rt lcd_intr_enable(LCD_con_t* lcon, uint32_t intr);
am18x_rt lcd_intr_disable(LCD_con_t* lcon, uint32_t intr);
am18x_rt lcd_intr_clear(LCD_con_t* lcon, uint32_t intr);
am18x_bool lcd_intr_state(const LCD_con_t* lcon, uint32_t intr);

#endif//__AM18X_LCD_H__
