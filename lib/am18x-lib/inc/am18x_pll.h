// tary, 17:27 2013/4/14

#ifndef __AM18X_PLL_H__
#define __AM18X_PLL_H__

#include "am18x_map.h"

typedef enum {
	PLL_CFLAG_EXT_CLK_OSCIN = 0x0,
	PLL_CFLAG_EXT_CLK_PLL1  = BIT(0),
	PLL_CFLAG_REF_CRYSTAL = 0x0,
	PLL_CFLAG_REF_SQUARE  = BIT(1),
	PLL_CFLAG_POWER_NORM = 0x0,
	PLL_CFLAG_POWER_DOWN = BIT(2),
	PLL_CFLAG_FROM_POWERON = BIT(3),
} PLL_CFLAG_flag_t;

typedef enum {
	PLL_CMD_SOFT_RESET,
	PLL_CMD_ENABLE_PLL1_DIVS,
	PLL_CMD_POWER_DOWN,
	PLL_CMD_BYPASS,
	PLL_CMD_CHG_MULT,
	PLL_CMD_UNBYPASS,
	PLL_CMD_IS_ENABLE,
} pll_cmd_t;

typedef enum {
	PLL_RESET_SOFTWARE,
	PLL_RESET_EXTERNAL,
	PLL_RESET_POWER_ON,
} pll_reset_t;

typedef struct {
	uint8_t		prediv;
	uint8_t		pllm;
	uint8_t		postdiv;
	uint8_t		plldiv[7];
	uint16_t	cflag;
} pll_conf_t;

am18x_rt pll_changing_sysclk_dividers(PLL_con_t* pcon, uint32_t plldivn, uint32_t divider);
am18x_rt pll_get_conf(const PLL_con_t* pcon, pll_conf_t* conf);
am18x_rt pll_set_conf(PLL_con_t* pcon, const pll_conf_t* conf);
am18x_rt pll_cmd(PLL_con_t* pcon, uint32_t cmd, uint32_t arg);
pll_reset_t pll_get_reset(void);

#endif//__AM18X_PLL_H__
