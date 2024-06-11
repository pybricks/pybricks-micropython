// tary, 21:18 2013/4/15
#include "am18x_pll.h"

#define PLL0_DIV_CNT		7
#define PLL1_DIV_CNT		3

// PLL0 plldivn = 1..7
// PLL1 plldivn = 1..3
am18x_rt pll_changing_sysclk_dividers(PLL_con_t* pcon, uint32_t plldivn, uint32_t divider) {
	uint32_t reg, msk, idx;

	// 1. Wait for the GOSTAT bit in PLLSTAT to clear to 0
	while (FIELD_GET(pcon->PLLSTAT, PLLSTAT_GOSTAT_MASK) != PLLSTAT_GOSTAT_done);

	// 2. Program the RATIO field in PLLDIVn
	msk = XXXDIVx_RATIO_MASK;
	if (plldivn <= PLL1_DIV_CNT) {
		idx = PLLDIVxA_IDX_1 + plldivn - 1;
		reg = pcon->PLLDIVxA[idx];
		reg = FIELD_SET(reg, XXXDIVx_DxEN_MASK, XXXDIVx_DxEN_enable);
		pcon->PLLDIVxA[idx] = FIELD_SET(reg, msk, XXXDIVx_RATIO_WR(divider));
	} else if (plldivn <= PLL0_DIV_CNT) {
		idx = PLLDIVxB_IDX_4 + plldivn - 4;
		reg = pcon->PLLDIVxB[idx];
		reg = FIELD_SET(reg, XXXDIVx_DxEN_MASK, XXXDIVx_DxEN_enable);
		pcon->PLLDIVxB[idx] = FIELD_SET(reg, msk, XXXDIVx_RATIO_WR(divider));
	}

	// 3. Set the GOSET bit in PLLCMD to 1
	pcon->PLLCMD = FIELD_SET(pcon->PLLCMD, PLLCMD_GOSET_MASK, PLLCMD_GOSET_initiate);

	// 4. Wait for the GOSTAT bit in PLLSTAT to clear to 0
	while (FIELD_GET(pcon->PLLSTAT, PLLSTAT_GOSTAT_MASK) != PLLSTAT_GOSTAT_done);

	return AM18X_OK;
}

am18x_rt pll_get_conf(const PLL_con_t* pcon, pll_conf_t* conf) {
	uint32_t msk, v;
	int i;

	conf->prediv = 1UL;
	if (pcon == PLL0) {
		conf->prediv += __field_xget(pcon->PREDIV, XXXDIVx_RATIO_MASK);
	}
	conf->pllm = 1UL + __field_xget(pcon->PLLM, PLLM_MASK);
	conf->postdiv = 1UL + __field_xget(pcon->POSTDIV, XXXDIVx_RATIO_MASK);

	for (i = 0; i < PLL0_DIV_CNT; i++) {
		if (i < PLL1_DIV_CNT) {
			v = pcon->PLLDIVxA[PLLDIVxA_IDX_1 + i];
			conf->plldiv[i] = 1UL + __field_xget(v, XXXDIVx_RATIO_MASK);
		} else if (pcon == PLL0) {
			v = pcon->PLLDIVxB[PLLDIVxB_IDX_4 + i - PLL1_DIV_CNT];
			conf->plldiv[i] = 1UL + __field_xget(v, XXXDIVx_RATIO_MASK);
		}
	}

	conf->cflag = 0;

	msk = PLLCTL_EXTCLKSRC_MASK;
	v = PLLCTL_EXTCLKSRC_PLL1sysclk3;
	if (pcon == PLL0 && FIELD_GET(pcon->PLLCTL, msk) == v) {
		conf->cflag |= PLL_CFLAG_EXT_CLK_PLL1;
	}

	msk = PLLCTL_CLKMODE_MASK;
	v = PLLCTL_CLKMODE_wave;
	if (pcon == PLL0 && FIELD_GET(pcon->PLLCTL, msk) == v) {
		conf->cflag |= PLL_CFLAG_REF_SQUARE;
	}

	msk = PLLCTL_PLLPWRDN_MASK;
	v = PLLCTL_PLLPWRDN_yes;
	if (FIELD_GET(pcon->PLLCTL, msk) == v) {
		conf->cflag |= PLL_CFLAG_POWER_DOWN;
	}

	return AM18X_OK;
}

am18x_rt pll_set_conf(PLL_con_t* pcon, const pll_conf_t* conf) {
	uint32_t msk, v;

	// 1. Program the CLKMODE bit in PLLC0 PLLCTL
	if (pcon == PLL0 && (conf->cflag & PLL_CFLAG_FROM_POWERON)) {
		msk = PLLCTL_CLKMODE_MASK;
		if (conf->cflag & PLL_CFLAG_REF_SQUARE) {
			v = PLLCTL_CLKMODE_wave;
		} else {
			v = PLLCTL_CLKMODE_crystal;
		}
		pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, v);
	}

	// 2. Switch the PLL to bypass mode
	// a) Clear the PLLENSRC bit in PLLCTL to 0
	//    allows PLLEN bit to take effect
	msk = PLLCTL_PLLENSRC_MASK;
	pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLENSRC_cleared);

	// b) For PLL0 only, select the clock source by programming
	//    the EXTCLKSRC bit in PLLCTL
	if (pcon == PLL0) {
		msk = PLLCTL_EXTCLKSRC_MASK;
		if (conf->cflag & PLL_CFLAG_EXT_CLK_PLL1) {
			v = PLLCTL_EXTCLKSRC_PLL1sysclk3;
		} else {
			v = PLLCTL_EXTCLKSRC_oscin;
		}
		pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, v);
	}

	// c) Clear the PLLEN bit in PLLCTL to 0
	msk = PLLCTL_PLLEN_MASK;
	pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLEN_no);

	// d) Wait for 4 OSCIN cycles to ensure that
	//    the PLLC has switch to bypass mode
	for (v = 0; v < 456 * 4 / 12; v++) {
		__asm volatile ("nop");
	}

	// 3. Clear PLLRST in PLLCTL to 0
	msk = PLLCTL_PLLRST_MASK;
	pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLRST_asserted);

	// 4. Clear the PLLPWRDN bit in PLLCTL to 0
	msk = PLLCTL_PLLPWRDN_MASK;
	if (FIELD_GET(pcon->PLLCTL, msk) == PLLCTL_PLLPWRDN_yes) {
		pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLPWRDN_no);
	}

	// 5. Program the desired multiplier value in PLLM.
	//    Program the POSTDIV, as needed.
	pcon->PLLM = FIELD_SET(0, PLLM_MASK, PLLM_WR(conf->pllm));
	msk = XXXDIVx_DxEN_MASK | XXXDIVx_RATIO_MASK;
	if (pcon == PLL0) {
		v = XXXDIVx_DxEN_enable | XXXDIVx_RATIO_WR(conf->prediv);
		pcon->PREDIV = FIELD_SET(0, msk, v);
	}
	v = XXXDIVx_DxEN_enable | XXXDIVx_RATIO_WR(conf->postdiv);
	pcon->POSTDIV = FIELD_SET(0, msk, v);

	// 6. If desired, program PLLDIVn registers to change
	//    the SYSCLKn divide values
	for (v = 0; v < PLL0_DIV_CNT; v++) {
		if (pcon == PLL1 && v >= PLL1_DIV_CNT) break;
		pll_changing_sysclk_dividers(pcon, v + 1, conf->plldiv[v]);
	}

	// 7. Set the PLLRST bit in PLL to 1 (brings PLL out of reset)
	msk = PLLCTL_PLLRST_MASK;
	pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLRST_deasserted);

	// 8. Wait for the PLL to lock
	// am1808.pdf 5.6.1 PLL Device-Specific Information
	// Max PLL Lock Time = 2000 * N / sqrt(M)
	// N = Pre-Divider Ratio
	// M = PLL Multiplier
	for (v = 0; v < 456 * ( 2000 * conf->prediv / 2) / 12; v++) {
		__asm volatile ("nop");
	}

	// 9. Set the PLLEN bit in PLLCTL to 1 (removes PLL from bypass mode)
	msk = PLLCTL_PLLEN_MASK;
	pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLEN_yes);

	return AM18X_OK;
}

am18x_rt pll_cmd(PLL_con_t* pcon, uint32_t cmd, uint32_t arg) {
	uint32_t reg, msk;
	uint32_t i;

	switch (cmd) {
	case PLL_CMD_SOFT_RESET:
		if (pcon != PLL0) break;
		pcon->RSCTRL = FIELD_SET(0, RSCTRL_KEY_MASK, RSCTRL_KEY_unlock);
		while (FIELD_GET(pcon->RSCTRL, RSCTRL_KEY_MASK) != RSCTRL_KEY_unlocked);
		pcon->RSCTRL = FIELD_SET(0, RSCTRL_SWRST_MASK, RSCTRL_SWRST_yes);
		break;

	case PLL_CMD_ENABLE_PLL1_DIVS:
		if (pcon != PLL1) break;
		msk = XXXDIVx_DxEN_MASK;
		for (i = 0; i < PLL1_DIV_CNT; i++) {
			int idx = PLLDIVxA_IDX_1 + i;
			reg = pcon->PLLDIVxA[idx];
			pcon->PLLDIVxA[idx] = FIELD_SET(reg, msk, XXXDIVx_DxEN_enable);
		}
		break;

	case PLL_CMD_POWER_DOWN:
		msk = PLLCTL_PLLEN_MASK;
		if (FIELD_GET(pcon->PLLCTL, msk) == PLLCTL_PLLEN_yes) {
			pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLEN_no);
		}
		msk = PLLCTL_PLLPWRDN_MASK;
		pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLPWRDN_yes);
		break;

	case PLL_CMD_BYPASS:
		msk = PLLCTL_PLLEN_MASK;
		pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLEN_no);
		for (i = 0; i < 456 * 4 / 12; i++) {
			__asm volatile ("nop");
		}
		break;

	case PLL_CMD_CHG_MULT:
		pcon->PLLM = FIELD_SET(0, PLLM_MASK, PLLM_WR(arg));
		for (i = 0; i < 456 * ( 2000 * (pcon->PREDIV + 1) / 2) / 12; i++) {
			__asm volatile ("nop");
		}
		break;

	case PLL_CMD_UNBYPASS:
		msk = PLLCTL_PLLEN_MASK;
		pcon->PLLCTL = FIELD_SET(pcon->PLLCTL, msk, PLLCTL_PLLEN_yes);
		break;

	case PLL_CMD_IS_ENABLE:
		msk = PLLCTL_PLLPWRDN_MASK;
		if (FIELD_GET(pcon->PLLCTL, msk) != PLLCTL_PLLPWRDN_no) {
			return AM18X_ERR;
		}
		msk = PLLCTL_PLLEN_MASK;
		if (FIELD_GET(pcon->PLLCTL, msk) != PLLCTL_PLLEN_yes) {
			return AM18X_ERR;
		}
		break;

	default:
		break;
	}
	
	return AM18X_OK;
}

pll_reset_t pll_get_reset(void) {
	PLL_con_t* pcon = PLL0;
	pll_reset_t r;

	r = PLL_RESET_SOFTWARE;
	if (FIELD_GET(pcon->RSTYPE, RSTYPE_XWRST_MASK) == RSTYPE_XWRST_yes) {
		r = PLL_RESET_EXTERNAL;
	}
	if (FIELD_GET(pcon->RSTYPE, RSTYPE_POR_MASK) == RSTYPE_POR_yes) {
		r = PLL_RESET_POWER_ON;
	}

	return r;
}
