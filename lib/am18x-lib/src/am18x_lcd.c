// tary, 22:33 2015/4/4
#include "am18x_lcd.h"
#include "am18x_dclk.h"
#include "auxlib.h"

#define WIDTH_2_PPL(w)		(((w) >> 4) - 1)

static am18x_rt lcd_set_pclk(LCD_con_t* lcon, uint32_t freq) {
	uint32_t lcd_clk;
	uint32_t reg, v;

	lcd_clk = dev_get_freq(DCLK_ID_LCDC);
	printk("LCD_CLK = %d\n", lcd_clk);
	v = lcd_clk / freq;

	reg = lcon->LCD_CTRL;
	lcon->LCD_CTRL = __field_xset(reg, LCD_CTRL_CLKDIV_MASK, v);
	return AM18X_OK;
}

am18x_rt lcd_conf(LCD_con_t* lcon, const lcd_conf_t* conf) {
	uint32_t reg, msk, v;

	// disable raster controller
	reg = lcon->RASTER_CTRL;
	reg = FIELD_SET(reg, RASTER_CTRL_EN_MASK, RASTER_CTRL_EN_no);
	lcon->RASTER_CTRL = reg;

	reg = lcon->LCD_CTRL;
	v = LCD_CTRL_MODESEL_Raster;
	lcon->LCD_CTRL = FIELD_SET(reg, LCD_CTRL_MODESEL_MASK, v);

	lcd_set_pclk(lcon, conf->pclk);

	reg = lcon->RASTER_TIMING_0;
	reg = __field_xset(reg, RT0_HFP_MASK, conf->hfp - 1);
	reg = __field_xset(reg, RT0_HBP_MASK, conf->hbp - 1);
	reg = __field_xset(reg, RT0_HSW_MASK, conf->hsw - 1);
	reg = __field_xset(reg, RT0_PPL_MASK, WIDTH_2_PPL(conf->width));
	lcon->RASTER_TIMING_0 = reg;

	reg = lcon->RASTER_TIMING_1;
	reg = __field_xset(reg, RT1_VFP_MASK, conf->vfp - 1);
	reg = __field_xset(reg, RT1_VBP_MASK, conf->vbp - 1);
	reg = __field_xset(reg, RT1_VSW_MASK, conf->vsw - 1);
	reg = __field_xset(reg, RT1_LPP_MASK, conf->height - 1);
	lcon->RASTER_TIMING_1 = reg;

	reg = lcon->RASTER_TIMING_2;
	msk = RT2_SYNCCTRL_MASK | RT2_SYNCEDGE_MASK;
	switch (conf->hvsync) {
	case LCD_HVSYNC_RISING:
		v = RT2_SYNCCTRL_Active | RT2_SYNCEDGE_Rising;
		break;
	case LCD_HVSYNC_FALLING:
		v = RT2_SYNCCTRL_Active | RT2_SYNCEDGE_Falling;
		break;
	case LCD_HVSYNC_PCLK:
	default:
		v = RT2_SYNCCTRL_Inactive;
		break;
	}
	reg = FIELD_SET(reg, msk, v);

	v = conf->cflag & LCD_CFLAG_BIAS_LOW? RT2_BIAS_low: RT2_BIAS_high;
	reg = FIELD_SET(reg, RT2_BIAS_MASK, v);
	v = conf->cflag & LCD_CFLAG_PIXEL_FALLING? RT2_IPC_falling: RT2_IPC_rising;
	reg = FIELD_SET(reg, RT2_IPC_MASK, v);
	v = conf->cflag & LCD_CFLAG_HSYNC_LOW? RT2_IHS_low: RT2_IHS_high;
	reg = FIELD_SET(reg, RT2_IHS_MASK, v);
	v = conf->cflag & LCD_CFLAG_VSYNC_LOW? RT2_IVS_low: RT2_IVS_high;
	reg = FIELD_SET(reg, RT2_IVS_MASK, v);
	reg = FIELD_SET(reg, RT2_ACBI_MASK, RT2_ACBI_VAL(0));
	reg = FIELD_SET(reg, RT2_ACB_MASK, RT2_ACB_VAL(0xFF));
	lcon->RASTER_TIMING_2 = reg;

	reg = lcon->RASTER_CTRL;
	reg = FIELD_SET(reg, RASTER_CTRL_STN565_MASK, RASTER_CTRL_STN565_disabled);
	reg = FIELD_SET(reg, RASTER_CTRL_ALTMAP_MASK, RASTER_CTRL_ALTMAP_D15_0);
	v = conf->bpp < 8? RASTER_CTRL_NIB_enabled: RASTER_CTRL_NIB_disabled;
	reg = FIELD_SET(reg, RASTER_CTRL_NIB_MASK, v);
	reg = FIELD_SET(reg, RASTER_CTRL_PLM_MASK, RASTER_CTRL_PLM_PaletteData);
	reg = FIELD_SET(reg, RASTER_CTRL_FDD_MASK, RASTER_CTRL_FDD_X(2));
	reg = FIELD_SET(reg, RASTER_CTRL_RDORDER_MASK, RASTER_CTRL_RDORDER_Little);
	reg = FIELD_SET(reg, RASTER_CTRL_TS_MASK, RASTER_CTRL_TS_TFT);
	reg = FIELD_SET(reg, RASTER_CTRL_MC_MASK, RASTER_CTRL_MC_Color);

	// disable all interrupt
	msk = RASTER_CTRL_FUFEN_MASK | RASTER_CTRL_SLEN_MASK | RASTER_CTRL_PLEN_MASK;
	msk |= RASTER_CTRL_DONEEN_MASK | RASTER_CTRL_ACEN_MASK;
	v = RASTER_CTRL_FUFEN_no | RASTER_CTRL_SLEN_no | RASTER_CTRL_PLEN_no;
	v |= RASTER_CTRL_DONEEN_no | RASTER_CTRL_ACEN_no;
	reg = FIELD_SET(reg, msk, v);
	lcon->RASTER_CTRL = reg;

	reg = lcon->LCDDMA_CTRL;
	reg = FIELD_SET(reg, LDMAC_TFR_MASK, LDMAC_TFR_128dwords);
	reg = FIELD_SET(reg, LDMAC_BURSTSIZE_MASK, LDMAC_BURSTSIZE_16);
	reg = FIELD_SET(reg, LDMAC_EOFINTEN_MASK, LDMAC_EOFINTEN_no);
	reg = FIELD_SET(reg, LDMAC_BIGENDIAN_MASK, LDMAC_BIGENDIAN_disabled);
	v = conf->fb1_base? LDMAC_FRAMEMODE_two: LDMAC_FRAMEMODE_one;
	reg = FIELD_SET(reg, LDMAC_FRAMEMODE_MASK, v);
	lcon->LCDDMA_CTRL = reg;

	lcon->LCDDMA_FB0_BASE = conf->fb0_base;
	v = conf->bpp == 8? 512: 32;				// Palette size
	v += conf->width * conf->height * conf->bpp >> 3;	// Pixels size
	v -= 2;
	lcon->LCDDMA_FB0_CEILING = conf->fb0_base + v;
	if (conf->fb1_base) {
		lcon->LCDDMA_FB1_BASE = conf->fb1_base;
		lcon->LCDDMA_FB1_CEILING = conf->fb1_base + v;
	}

	return AM18X_OK;
}

am18x_rt lcd_cmd(LCD_con_t* lcon, uint32_t cmd, uint32_t arg) {
	uint32_t reg;

	switch(cmd) {
	case LCD_CMD_RASTER_EN:
		reg = lcon->RASTER_CTRL;
		reg = FIELD_SET(reg, RASTER_CTRL_EN_MASK, RASTER_CTRL_EN_yes);
		lcon->RASTER_CTRL = reg;
		break;
	case LCD_CMD_RASTER_DIS:
		reg = lcon->RASTER_CTRL;
		reg = FIELD_SET(reg, RASTER_CTRL_EN_MASK, RASTER_CTRL_EN_no);
		lcon->RASTER_CTRL = reg;
		break;
	case LCD_CMD_PALETTE:
		reg = lcon->RASTER_CTRL;
		reg = FIELD_SET(reg, RASTER_CTRL_PLM_MASK, RASTER_CTRL_PLM_Palette);
		lcon->RASTER_CTRL = reg;
		break;
	case LCD_CMD_DATA:
		reg = lcon->RASTER_CTRL;
		reg = FIELD_SET(reg, RASTER_CTRL_PLM_MASK, RASTER_CTRL_PLM_Data);
		lcon->RASTER_CTRL = reg;
		break;
	case LCD_CMD_FB_SET:
		if (arg == 0) {
			lcon->LCDDMA_FB0_BASE = lcon->LCDDMA_FB0_BASE;
		} else {
			lcon->LCDDMA_FB0_BASE = arg;
		}
		break;
	default:
		break;
	}
	return AM18X_OK;
}

am18x_rt lcd_intr_enable(LCD_con_t* lcon, uint32_t intr) {
	uint32_t reg, msk, v;

	if (intr == LCD_INTR_EOF) {
		reg = lcon->LCDDMA_CTRL;
		reg = FIELD_SET(reg, LDMAC_EOFINTEN_MASK, LDMAC_EOFINTEN_yes);
		lcon->LCDDMA_CTRL = reg;
		return AM18X_OK;
	}

	reg = lcon->RASTER_CTRL;
	msk = 0;
	v = 0;
	switch(intr) {
	case LCD_INTR_AC:
		msk = RASTER_CTRL_ACEN_MASK;
		v = RASTER_CTRL_ACEN_yes;
		break;
	case LCD_INTR_DONE:
		msk = RASTER_CTRL_DONEEN_MASK;
		v = RASTER_CTRL_DONEEN_yes;
		break;
	case LCD_INTR_PL:
		msk = RASTER_CTRL_PLEN_MASK;
		v = RASTER_CTRL_PLEN_yes;
		break;
	case LCD_INTR_SL:
		msk = RASTER_CTRL_SLEN_MASK;
		v = RASTER_CTRL_SLEN_yes;
		break;
	case LCD_INTR_FUF:
		msk = RASTER_CTRL_FUFEN_MASK;
		v = RASTER_CTRL_FUFEN_yes;
		break;
	default:
		break;
	}
	reg = FIELD_SET(reg, msk, v);
	lcon->RASTER_CTRL = reg;

	return AM18X_OK;
}

am18x_rt lcd_intr_disable(LCD_con_t* lcon, uint32_t intr) {
	uint32_t reg, msk, v;

	if (intr == LCD_INTR_EOF) {
		reg = lcon->LCDDMA_CTRL;
		reg = FIELD_SET(reg, LDMAC_EOFINTEN_MASK, LDMAC_EOFINTEN_no);
		lcon->LCDDMA_CTRL = reg;
		return AM18X_OK;
	}

	reg = lcon->RASTER_CTRL;
	msk = 0;
	v = 0;
	switch(intr) {
	case LCD_INTR_AC:
		msk = RASTER_CTRL_ACEN_MASK;
		v = RASTER_CTRL_ACEN_no;
		break;
	case LCD_INTR_DONE:
		msk = RASTER_CTRL_DONEEN_MASK;
		v = RASTER_CTRL_DONEEN_no;
		break;
	case LCD_INTR_PL:
		msk = RASTER_CTRL_PLEN_MASK;
		v = RASTER_CTRL_PLEN_no;
		break;
	case LCD_INTR_SL:
		msk = RASTER_CTRL_SLEN_MASK;
		v = RASTER_CTRL_SLEN_no;
		break;
	case LCD_INTR_FUF:
		msk = RASTER_CTRL_FUFEN_MASK;
		v = RASTER_CTRL_FUFEN_no;
		break;
	default:
		break;
	}
	reg = FIELD_SET(reg, msk, v);
	lcon->RASTER_CTRL = reg;

	return AM18X_OK;
}

am18x_rt lcd_intr_clear(LCD_con_t* lcon, uint32_t intr) {
	uint32_t msk, v;

	msk = 0;
	v = 0;
	switch(intr) {
	case LCD_INTR_AC:
		msk = LCD_STAT_ABC_MASK;
		v = LCD_STAT_ABC_zero;
		break;
	case LCD_INTR_DONE:
		msk = LCD_STAT_DONE_MASK;
		v = LCD_STAT_DONE_yes;
		break;
	case LCD_INTR_PL:
		msk = LCD_STAT_PL_MASK;
		v = LCD_STAT_PL_yes;
		break;
	case LCD_INTR_SL:
		msk = LCD_STAT_SYNC_MASK;
		v = LCD_STAT_SYNC_lost;
		break;
	case LCD_INTR_FUF:
		msk = LCD_STAT_FUF_MASK;
		v = LCD_STAT_FUF_yes;
		break;
	case LCD_INTR_EOF:
		msk = LCD_STAT_EOF0_MASK;
		v = LCD_STAT_EOF0_yes;
		break;
	case LCD_INTR_EOF1:
		msk = LCD_STAT_EOF1_MASK;
		v = LCD_STAT_EOF1_yes;
		break;
	case LCD_INTR_ALL:
		msk = 0xFFFFFFFFUL;
		v = lcon->LCD_STAT;
		break;
	default:
		break;
	}
	lcon->LCD_STAT = FIELD_SET(0, msk, v);;

	return AM18X_OK;
}

am18x_bool lcd_intr_state(const LCD_con_t* lcon, uint32_t intr) {
	uint32_t msk, v;
	am18x_bool r = AM18X_FALSE;

	switch(intr) {
	case LCD_INTR_AC:
		msk = LCD_STAT_ABC_MASK;
		v = LCD_STAT_ABC_zero;
		break;
	case LCD_INTR_DONE:
		msk = LCD_STAT_DONE_MASK;
		v = LCD_STAT_DONE_yes;
		break;
	case LCD_INTR_PL:
		msk = LCD_STAT_PL_MASK;
		v = LCD_STAT_PL_yes;
		break;
	case LCD_INTR_SL:
		msk = LCD_STAT_SYNC_MASK;
		v = LCD_STAT_SYNC_lost;
		break;
	case LCD_INTR_FUF:
		msk = LCD_STAT_FUF_MASK;
		v = LCD_STAT_FUF_yes;
		break;
	case LCD_INTR_EOF:
		msk = LCD_STAT_EOF0_MASK;
		v = LCD_STAT_EOF0_yes;
		break;
	case LCD_INTR_EOF1:
		msk = LCD_STAT_EOF1_MASK;
		v = LCD_STAT_EOF1_yes;
		break;
	default:
		return r;
	}
	if (FIELD_GET(lcon->LCD_STAT, msk) == v) {
		r = AM18X_TRUE;
	}

	return r;
}
