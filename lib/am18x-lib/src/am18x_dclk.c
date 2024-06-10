// tary, 19:05 2013/4/20
#include "am18x_dclk.h"
#include "auxlib.h"

static clk_node_t clk_nodes[];

#define cfdc_pllx_sysclk_1_3(pll_nr, s_nr)					\
static uint32_t calc_freq_PLL##pll_nr##_SYSCLK##s_nr (uint32_t parent) {	\
	uint32_t id = CLK_NODE_PLL##pll_nr##_SYSCLK##s_nr;			\
	uint32_t reg = PLL##pll_nr->PLLDIVxA[PLLDIVxA_IDX_##s_nr];		\
	uint32_t msk = XXXDIVx_DxEN_MASK;					\
										\
	if (FIELD_GET(reg, msk) == XXXDIVx_DxEN_disable) {			\
		clk_nodes[id].parent = CLK_NODE_INVALID;			\
	} else {								\
		clk_nodes[id].parent = CLK_NODE_PLL##pll_nr##_PLLEN;		\
	}									\
	msk = XXXDIVx_RATIO_MASK;						\
	clk_nodes[id].divider = 1UL + __field_xget(reg, msk);			\
	return 0;								\
}										\
static uint32_t do_change_PLL##pll_nr##_SYSCLK##s_nr (uint32_t parent) {	\
	uint32_t id = CLK_NODE_PLL##pll_nr##_SYSCLK##s_nr;			\
	uint32_t msk = XXXDIVx_DxEN_MASK;					\
	uint32_t reg, divider;							\
										\
	divider = clk_nodes[id].divider;					\
	(void)divider; \
	reg = PLL##pll_nr->PLLDIVxA[PLLDIVxA_IDX_##s_nr];			\
	if (clk_nodes[id].parent == CLK_NODE_PLL##pll_nr##_PLLEN) {		\
		reg = FIELD_SET(reg, msk, XXXDIVx_DxEN_enable);			\
	} else if (clk_nodes[id].parent == CLK_NODE_INVALID) {			\
		reg = FIELD_SET(reg, msk, XXXDIVx_DxEN_disable);		\
	}									\
	PLL##pll_nr->PLLDIVxA[PLLDIVxA_IDX_##s_nr] = reg;			\
	return 0;								\
}

#define cfdc_pllx_sysclk_4_7(pll_nr, s_nr)					\
static uint32_t calc_freq_PLL##pll_nr##_SYSCLK##s_nr (uint32_t parent) {	\
	uint32_t id = CLK_NODE_PLL##pll_nr##_SYSCLK##s_nr;			\
	uint32_t reg = PLL##pll_nr->PLLDIVxB[PLLDIVxB_IDX_##s_nr];		\
	uint32_t msk = XXXDIVx_DxEN_MASK;					\
										\
	if (FIELD_GET(reg, msk) == XXXDIVx_DxEN_disable) {			\
		clk_nodes[id].parent = CLK_NODE_INVALID;			\
	} else {								\
		clk_nodes[id].parent = CLK_NODE_PLL##pll_nr##_PLLEN;		\
	}									\
	msk = XXXDIVx_RATIO_MASK;						\
	clk_nodes[id].divider = 1UL + __field_xget(reg, msk);			\
	return 0;								\
}										\
static uint32_t do_change_PLL##pll_nr##_SYSCLK##s_nr (uint32_t parent) {	\
	uint32_t id = CLK_NODE_PLL##pll_nr##_SYSCLK##s_nr;			\
	uint32_t msk = XXXDIVx_DxEN_MASK;					\
	uint32_t reg, divider;							\
										\
	divider = clk_nodes[id].divider;					\
	(void)divider; \
	reg = PLL##pll_nr->PLLDIVxB[PLLDIVxB_IDX_##s_nr];			\
	if (clk_nodes[id].parent == CLK_NODE_PLL##pll_nr##_PLLEN) {		\
		reg = FIELD_SET(reg, msk, XXXDIVx_DxEN_enable);			\
	} else if (clk_nodes[id].parent == CLK_NODE_INVALID) {			\
		reg = FIELD_SET(reg, msk, XXXDIVx_DxEN_disable);		\
	}									\
	PLL##pll_nr->PLLDIVxB[PLLDIVxB_IDX_##s_nr] = reg;			\
	return 0;								\
}

#define cfdc_switch(name, cntrl, r, m, vdis, ven, ndis, nen)			\
static uint32_t calc_freq_##name (uint32_t parent) {				\
	uint32_t id = CLK_NODE_##name;						\
	if (FIELD_GET(XX##cntrl->XX##r,r##_##m) == r##_##vdis ) {		\
		clk_nodes[id].parent = CLK_NODE_##ndis;				\
	} else {								\
		clk_nodes[id].parent = CLK_NODE_##nen;				\
	}									\
	return 0;								\
}										\
static uint32_t do_change_##name (uint32_t parent) {				\
	uint32_t id = CLK_NODE_##name;						\
	uint32_t reg = XX##cntrl->XX##r;					\
	uint32_t v;								\
	if (clk_nodes[id].parent == CLK_NODE_##ndis) {				\
		v = r##_##vdis;							\
	} else {								\
		v = r##_##ven;							\
	}									\
	XX##cntrl->XX##r = FIELD_SET(reg, r##_##m, v);				\
	return 0;								\
}

#define cfdc_pllx_mult(r, pll_nr)						\
static uint32_t calc_freq_##r##pll_nr (uint32_t parent) {			\
	uint32_t id = CLK_NODE_##r##pll_nr;					\
	uint32_t msk = XXXDIVx_RATIO_MASK;					\
										\
	clk_nodes[id].multiplier = 1UL + __field_xget(PLL##pll_nr->XX##r, msk);	\
	return 0;								\
}										\
static uint32_t do_change_##r##pll_nr (uint32_t parent) {			\
	uint32_t id = CLK_NODE_##r##pll_nr;					\
	uint32_t msk = XXXDIVx_RATIO_MASK;					\
	uint32_t reg, rate;							\
										\
	rate = clk_nodes[id].multiplier;					\
	reg = PLL##pll_nr->XX##r;						\
	PLL##pll_nr->XX##r = FIELD_SET(reg, msk, XXXDIVx_RATIO_WR(rate));	\
	return 0;								\
}

#define cfdc_pllx_xxxdiv(r, pll_nr, par)					\
static uint32_t calc_freq_##r##pll_nr (uint32_t parent) {			\
	uint32_t id = CLK_NODE_##r##pll_nr;					\
	uint32_t reg = PLL##pll_nr->XX##r;					\
	uint32_t msk = XXXDIVx_DxEN_MASK;					\
										\
	if (FIELD_GET(reg, msk) == XXXDIVx_DxEN_disable) {			\
		clk_nodes[id].parent = CLK_NODE_INVALID;			\
	} else {								\
		clk_nodes[id].parent = CLK_NODE_##par;				\
	}									\
	msk = XXXDIVx_RATIO_MASK;						\
	clk_nodes[id].divider = 1UL + __field_xget(reg, msk);			\
	return 0;								\
}										\
static uint32_t do_change_##r##pll_nr (uint32_t parent) {			\
	uint32_t id = CLK_NODE_##r##pll_nr;					\
	uint32_t msk = XXXDIVx_DxEN_MASK;					\
	uint32_t reg, divider;							\
										\
	divider = clk_nodes[id].divider;					\
	reg = PLL##pll_nr->XX##r;						\
	if (clk_nodes[id].parent == CLK_NODE_##par) {				\
		reg = FIELD_SET(reg, msk, XXXDIVx_DxEN_enable);			\
	} else if (clk_nodes[id].parent == CLK_NODE_INVALID) {			\
		reg = FIELD_SET(reg, msk, XXXDIVx_DxEN_disable);		\
	}									\
	msk = XXXDIVx_RATIO_MASK;						\
	reg = FIELD_SET(reg, msk, XXXDIVx_RATIO_WR(divider));			\
	PLL##pll_nr->XX##r = reg;						\
	return 0;								\
}

cfdc_pllx_sysclk_1_3(0,1)
cfdc_pllx_sysclk_1_3(0,2)
cfdc_pllx_sysclk_1_3(0,3)
cfdc_pllx_sysclk_4_7(0,4)
cfdc_pllx_sysclk_4_7(0,5)
cfdc_pllx_sysclk_4_7(0,6)
cfdc_pllx_sysclk_4_7(0,7)
cfdc_pllx_sysclk_1_3(1,1)
cfdc_pllx_sysclk_1_3(1,2)
cfdc_pllx_sysclk_1_3(1,3)
#define XXPLL0    PLL0
#define XXPLL1    PLL1
#define XXCKEN    CKEN
#define XXPLLCTL  PLLCTL
#define XXPREDIV  PREDIV
#define XXPLLM    PLLM
#define XXPOSTDIV POSTDIV
#define XXOSCDIV  OSCDIV
cfdc_switch(PLL0_AUXCLK,PLL0,CKEN,AUXEN_MASK,AUXEN_disable,AUXEN_enable,INVALID,PLL_CLKMODE)
cfdc_switch(PLL0_OBSCLK,PLL0,CKEN,OBSEN_MASK,OBSEN_disable,OBSEN_enable,INVALID,OSCDIV0)
cfdc_switch(PLL1_OBSCLK,PLL1,CKEN,OBSEN_MASK,OBSEN_disable,OBSEN_enable,INVALID,OSCDIV1)
cfdc_switch(PLL0_PLLEN,PLL0,PLLCTL,PLLEN_MASK,PLLEN_no,PLLEN_yes,PLL_EXTSRC,POSTDIV0)
cfdc_switch(PLL1_PLLEN,PLL1,PLLCTL,PLLEN_MASK,PLLEN_no,PLLEN_yes,PLL_CLKMODE,POSTDIV1)
cfdc_switch(PLL_EXTSRC,PLL0,PLLCTL,EXTCLKSRC_MASK,EXTCLKSRC_oscin,EXTCLKSRC_PLL1sysclk3,PLL_CLKMODE,PLL1_SYSCLK3)
#define XXSYSCFG0 SYSCFG0
#define XXCFGCHIP3 CFGCHIP3
cfdc_switch(EMA_CLKSRC,SYSCFG0,CFGCHIP3,EMA_CLKSRC_MASK,EMA_CLKSRC_sysclk3,EMA_CLKSRC_pll_out,PLL0_SYSCLK3,DIV4_5X)
cfdc_switch(ASYNC3,SYSCFG0,CFGCHIP3,ASYNC3_CLKSRC_MASK,ASYNC3_CLKSRC_pll0,ASYNC3_CLKSRC_pll1,PLL0_SYSCLK2,PLL1_SYSCLK2)
cfdc_switch(DIV4_5X,SYSCFG0,CFGCHIP3,DIV45PENA_MASK,DIV45PENA_no,DIV45PENA_yes,INVALID,DIV4_5)
cfdc_pllx_mult(PLLM,0)
cfdc_pllx_mult(PLLM,1)
cfdc_pllx_xxxdiv(PREDIV,0,PLL_CLKMODE)
cfdc_pllx_xxxdiv(POSTDIV,0,PLLM0)
cfdc_pllx_xxxdiv(OSCDIV,0,OCSEL0_OCSRC)
cfdc_pllx_xxxdiv(POSTDIV,1,PLLM1)
cfdc_pllx_xxxdiv(OSCDIV,1,OCSEL1_OCSRC)

static uint32_t calc_freq_OCSEL0_OCSRC (uint32_t parent) {
	uint32_t id = CLK_NODE_OCSEL0_OCSRC;
	uint32_t par;

	switch (FIELD_GET(PLL0->OCSEL,OCSEL_OCSRC_MASK)) {
	case OCSEL_OCSRC_PLLsysclkx(1):
		par = CLK_NODE_PLL0_SYSCLK1;
		break;
	case OCSEL_OCSRC_PLLsysclkx(2):
		par = CLK_NODE_PLL0_SYSCLK2;
		break;
	case OCSEL_OCSRC_PLLsysclkx(3):
		par = CLK_NODE_PLL0_SYSCLK3;
		break;
	case OCSEL_OCSRC_PLLsysclkx(4):
		par = CLK_NODE_PLL0_SYSCLK4;
		break;
	case OCSEL_OCSRC_PLLsysclkx(5):
		par = CLK_NODE_PLL0_SYSCLK5;
		break;
	case OCSEL_OCSRC_PLLsysclkx(6):
		par = CLK_NODE_PLL0_SYSCLK6;
		break;
	case OCSEL_OCSRC_PLLsysclkx(7):
		par = CLK_NODE_PLL0_SYSCLK7;
		break;
	case OCSEL_OCSRC_PLL1obsclk:
		par = CLK_NODE_PLL1_OBSCLK;
		break;
	case OCSEL_OCSRC_oscin:
		par = CLK_NODE_PLL_CLKMODE;
		break;
	case OCSEL_OCSRC_Disabled:
	default:
		par = CLK_NODE_INVALID;
		break;
	}
	clk_nodes[id].parent = par;
	return 0;
}
static uint32_t do_change_OCSEL0_OCSRC (uint32_t parent) {
	uint32_t id = CLK_NODE_OCSEL0_OCSRC;
	uint32_t v;

	switch (clk_nodes[id].parent) {
	case CLK_NODE_PLL0_SYSCLK1:
		v = OCSEL_OCSRC_PLLsysclkx(1);
		break;
	case CLK_NODE_PLL0_SYSCLK2:
		v = OCSEL_OCSRC_PLLsysclkx(2);
		break;
	case CLK_NODE_PLL0_SYSCLK3:
		v = OCSEL_OCSRC_PLLsysclkx(3);
		break;
	case CLK_NODE_PLL0_SYSCLK4:
		v = OCSEL_OCSRC_PLLsysclkx(4);
		break;
	case CLK_NODE_PLL0_SYSCLK5:
		v = OCSEL_OCSRC_PLLsysclkx(5);
		break;
	case CLK_NODE_PLL0_SYSCLK6:
		v = OCSEL_OCSRC_PLLsysclkx(6);
		break;
	case CLK_NODE_PLL0_SYSCLK7:
		v = OCSEL_OCSRC_PLLsysclkx(7);
		break;
	case CLK_NODE_PLL1_OBSCLK:
		v = OCSEL_OCSRC_PLL1obsclk;
		break;
	case CLK_NODE_PLL_CLKMODE:
		v = OCSEL_OCSRC_oscin;
		break;
	case CLK_NODE_INVALID:
	default:
		v = OCSEL_OCSRC_Disabled;
	}
	PLL0->OCSEL = FIELD_SET(PLL0->OCSEL, OCSEL_OCSRC_MASK, v);
	return 0;
}

static uint32_t calc_freq_OCSEL1_OCSRC (uint32_t parent) {
	uint32_t id = CLK_NODE_OCSEL1_OCSRC;
	uint32_t par;

	switch (FIELD_GET(PLL1->OCSEL,OCSEL_OCSRC_MASK)) {
	case OCSEL_OCSRC_PLLsysclkx(1):
		par = CLK_NODE_PLL1_SYSCLK1;
		break;
	case OCSEL_OCSRC_PLLsysclkx(2):
		par = CLK_NODE_PLL1_SYSCLK2;
		break;
	case OCSEL_OCSRC_PLLsysclkx(3):
		par = CLK_NODE_PLL1_SYSCLK3;
		break;
	case OCSEL_OCSRC_oscin:
	default:
		par = CLK_NODE_PLL_CLKMODE;
		break;
	}
	clk_nodes[id].parent = par;
	return 0;
}
static uint32_t do_change_OCSEL1_OCSRC (uint32_t parent) {
	uint32_t id = CLK_NODE_OCSEL1_OCSRC;
	uint32_t v;

	switch (clk_nodes[id].parent) {
	case CLK_NODE_PLL1_SYSCLK1:
		v = OCSEL_OCSRC_PLLsysclkx(1);
		break;
	case CLK_NODE_PLL1_SYSCLK2:
		v = OCSEL_OCSRC_PLLsysclkx(2);
		break;
	case CLK_NODE_PLL1_SYSCLK3:
		v = OCSEL_OCSRC_PLLsysclkx(3);
		break;
	case CLK_NODE_PLL_CLKMODE:
	default:
		v = OCSEL_OCSRC_oscin;
		break;
	}
	PLL1->OCSEL = FIELD_SET(PLL1->OCSEL, OCSEL_OCSRC_MASK, v);
	return 0;
}

#define cnm(name)	CLK_NODE_##name, #name
#define cm(name)	CLK_NODE_##name
#define cfdc(name)	calc_freq_##name, do_change_##name
#define CFMUX		CN_FLAG_MUX
#define REREAD		CN_FLAG_REREAD
#define CFDC_VALID(flg)	((flg) & (CN_FLAG_MUX | CN_FLAG_REREAD))
#define RECALC		CN_FLAG_RECALC

static clk_node_t clk_nodes[] = {
	// ID                  PARENT,       FLAG, MULT,DIV, CALC_FREQ, DO_CHANGE, PARENT_LIST
	{ cnm(INVALID),      cm(INVALID),          0, 0, 0, NULL, NULL        , NULL, 0},
	{ cnm(PLL0_SYSCLK1), cm(PLL0_PLLEN),  REREAD, 0, 1, cfdc(PLL0_SYSCLK1), NULL, 0},
	{ cnm(PLL0_SYSCLK2), cm(PLL0_PLLEN),  REREAD, 0, 2, cfdc(PLL0_SYSCLK2), NULL, 0},
	{ cnm(PLL0_SYSCLK3), cm(PLL0_PLLEN),  REREAD, 0, 3, cfdc(PLL0_SYSCLK3), NULL, 0},
	{ cnm(PLL0_SYSCLK4), cm(PLL0_PLLEN),  REREAD, 0, 4, cfdc(PLL0_SYSCLK4), NULL, 0},
	{ cnm(PLL0_SYSCLK5), cm(PLL0_PLLEN),  REREAD, 0, 3, cfdc(PLL0_SYSCLK5), NULL, 0},
	{ cnm(PLL0_SYSCLK6), cm(PLL0_PLLEN),  REREAD, 0, 1, cfdc(PLL0_SYSCLK6), NULL, 0},
	{ cnm(PLL0_SYSCLK7), cm(PLL0_PLLEN),  REREAD, 0, 6, cfdc(PLL0_SYSCLK7), NULL, 0},
	{ cnm(EMA_CLKSRC),   cm(PLL0_SYSCLK3), CFMUX, 0, 0, cfdc(EMA_CLKSRC),   NULL, 0},
	{ cnm(PLL0_AUXCLK),  cm(PLL_CLKMODE),  CFMUX, 0, 0, cfdc(PLL0_AUXCLK),  NULL, 0},
	{ cnm(PLL0_OBSCLK),  cm(OSCDIV0),      CFMUX, 0, 0, cfdc(PLL0_OBSCLK),  NULL, 0},
	{ cnm(PLL1_SYSCLK1), cm(PLL1_PLLEN),  REREAD, 0, 1, cfdc(PLL1_SYSCLK1), NULL, 0},
	{ cnm(PLL1_SYSCLK2), cm(PLL1_PLLEN),  REREAD, 0, 2, cfdc(PLL1_SYSCLK2), NULL, 0},
	{ cnm(PLL1_SYSCLK3), cm(PLL1_PLLEN),  REREAD, 0, 3, cfdc(PLL1_SYSCLK3), NULL, 0},
	{ cnm(PLL1_OBSCLK),  cm(OSCDIV1),      CFMUX, 0, 0, cfdc(PLL1_OBSCLK),  NULL, 0},
	{ cnm(ASYNC3),       cm(PLL0_SYSCLK2), CFMUX, 0, 0, cfdc(ASYNC3),       NULL, 0},
	{ cnm(PLL0_PLLEN),   cm(PLL_EXTSRC),   CFMUX, 0, 0, cfdc(PLL0_PLLEN),   NULL, 0},
	{ cnm(PLL_EXTSRC),   cm(PLL_CLKMODE),  CFMUX, 0, 0, cfdc(PLL_EXTSRC),   NULL, 0},
	{ cnm(POSTDIV0),     cm(PLLM0),       REREAD, 0, 0, cfdc(POSTDIV0),     NULL, 0},
	{ cnm(PLLM0),        cm(PREDIV0),     REREAD, 0, 0, cfdc(PLLM0),        NULL, 0},
	{ cnm(PREDIV0),      cm(PLL_CLKMODE), REREAD, 0, 0, cfdc(PREDIV0),      NULL, 0},
	{ cnm(PLL_CLKMODE),  cm(OSCIN),            0, 1, 0, NULL, NULL        , NULL, 0},
	{ cnm(DIV4_5X),      cm(DIV4_5),       CFMUX, 0, 0, cfdc(DIV4_5X),      NULL, 0},
	{ cnm(DIV4_5),       cm(PLLM0),            0, 2, 9, NULL, NULL        , NULL, 0},
	{ cnm(OSCDIV0),      cm(OCSEL0_OCSRC),REREAD, 0, 0, cfdc(OSCDIV0),      NULL, 0},
	{ cnm(OCSEL0_OCSRC), cm(PLL_CLKMODE),  CFMUX, 0, 0, cfdc(OCSEL0_OCSRC), NULL, 0},
	{ cnm(PLL1_PLLEN),   cm(POSTDIV1),     CFMUX, 0, 0, cfdc(PLL1_PLLEN),   NULL, 0},
	{ cnm(POSTDIV1),     cm(PLLM1),       REREAD, 0, 0, cfdc(POSTDIV1),     NULL, 0},
	{ cnm(PLLM1),        cm(PLL_CLKMODE), REREAD, 0, 0, cfdc(PLLM1),        NULL, 0},
	{ cnm(OSCDIV1),      cm(OCSEL1_OCSRC),REREAD, 0, 0, cfdc(OSCDIV1),      NULL, 0},
	{ cnm(OCSEL1_OCSRC), cm(PLL_CLKMODE),  CFMUX, 0, 0, cfdc(OCSEL1_OCSRC), NULL, 0},
	{ cnm(OSCIN),	     cm(INVALID),          0, 0, 0, NULL, NULL        , NULL, 0},
};

am18x_rt clk_node_init(void) {
	uint32_t i;

	for (i = 0; i < countof(clk_nodes); i++) {
		clk_node_t* cni = clk_nodes + i;

		cni->flag |= CN_FLAG_RECALC;
	}
	return AM18X_OK;
}

static uint32_t clk_node_calc_freq_inner(uint32_t id) {
	uint32_t freq;
	clk_node_t* cni = clk_nodes + id;

	#if 0
	printk("%d ", id);
	#endif

	if (id == CLK_NODE_INVALID) {
		return 0UL;
	}
	if (id == CLK_NODE_OSCIN) {
		return F_OSCIN;
	}
	if ((cni->flag & CN_FLAG_RECALC) == 0) {
		return cni->freq;
	}

	if (CFDC_VALID(cni->flag)) {
		(cni->calc_freq)(0);
	}
	
	freq = clk_node_calc_freq_inner(cni->parent);
	if ((cni->flag & CN_FLAG_MUX) == 0) {
		if (cni->multiplier != 0) freq *= cni->multiplier;
		if (cni->divider != 0) freq /= cni->divider;
	}

	if (cni->flag & CN_FLAG_RECALC) {
		cni->flag &= ~CN_FLAG_RECALC;
		cni->freq = freq;
	}
	return freq;
}

am18x_rt clk_node_recalc(void) {
	int i;

	for (i = CLK_NODE_INVALID + 1; i < CLK_NODE_CNT; i++) {
		clk_node_t* cni = clk_nodes + i;
		cni->flag |= CN_FLAG_RECALC;
		cni->flag &= ~CN_FLAG_VISITED;
	}
	for (i = CLK_NODE_INVALID + 1; i < CLK_NODE_CNT; i++) {
		clk_node_calc_freq_inner(i);
	}
	return AM18X_OK;
}

uint32_t clk_node_get_freq(uint32_t id) {
	uint32_t freq;

	freq = clk_node_calc_freq_inner(id);

	return freq;
}

am18x_rt clk_node_output(void) {
	#define ONE_MEGA	1000000UL
	#define ONE_KILO	1000UL
	int i;

	clk_node_recalc();

	for (i = CLK_NODE_INVALID + 1; i < CLK_NODE_CNT; i++) {
		uint32_t f = clk_node_get_freq(i);

		printk("[%12s] = ", clk_nodes[i].name);
		if (f % ONE_MEGA == 0) {
			printk("%10dMhz\n", f / ONE_MEGA);
		} else if (f < ONE_MEGA) {
			uint32_t frac = f % ONE_KILO;
			printk("%6d.%3dKhz\n", f / ONE_KILO, frac);
		} else {
			uint32_t frac = f % ONE_MEGA / 1000;
			printk("%6d.%3dMhz\n", f / ONE_MEGA, frac);
		}
	}
	return AM18X_OK;
}

static uint32_t clk_node_tree_innner(uint32_t id, int level) {
	#define LINE_SIZE	1024
	#define LINE_UNIT	14
	static char line0[LINE_SIZE];
	static char line1[LINE_SIZE];
	uint32_t freq;
	am18x_bool found = AM18X_FALSE;
	int i;

	freq = clk_node_get_freq(id);

	sprintf(line0 + LINE_UNIT * level, "[%12s]", clk_nodes[id].name);
	sprintf(line1 + LINE_UNIT * level, "    %4dMhz   ", (int)(freq / ONE_MEGA));

	for (i = CLK_NODE_INVALID + 1; i < CLK_NODE_CNT; i++) {
		if (clk_nodes[i].parent == id) {
			clk_node_tree_innner(i, level + 1);
			found = AM18X_TRUE;
		}
	}

	if (found) {
		return 0;
	}

	for (i = level; i >= 0; i--) {
		if (line0[i * LINE_UNIT] != '[') {
			break;
		}
	}
	if (i >= 0) {
		int j = i * LINE_UNIT + LINE_UNIT / 2 + 1;

		line0[j++] = '|';
		for (; j < (i + 1) * LINE_UNIT; j++) {
			line0[j] = '_';
		}
	}
	printk("%s\n", line0);
	printk("%s\n", line1);

	for (i = 0; i < level * LINE_UNIT; i++) {
		line0[i] = line1[i] = ' ';
	}
	return 0;
}

am18x_rt clk_node_tree(void) {
	int i;

	clk_node_recalc();

	for (i = CLK_NODE_INVALID + 1; i < CLK_NODE_CNT; i++) {
		clk_node_t* cni = clk_nodes + i;

		if (cni->parent != CLK_NODE_INVALID) {
			continue;
		}
		clk_node_tree_innner(i, 0);
	}
	return AM18X_OK;
}

static am18x_bool clk_node_set_parent_inner(uint32_t id) {
	am18x_bool found = AM18X_FALSE;
	int i;

	clk_nodes[id].flag |= CN_FLAG_RECALC;

	for (i = CLK_NODE_INVALID + 1; i < CLK_NODE_CNT; i++) {
		if (clk_nodes[i].parent == id) {
			clk_node_set_parent_inner(i);
			found = AM18X_TRUE;
		}
	}

	return found;
}

am18x_rt clk_node_set_parent(uint32_t id, uint32_t parent) {
	clk_node_t* cni = clk_nodes + id;

	if (CFDC_VALID(cni->flag) == 0) {
		return AM18X_ERR;
	}

	cni->parent = parent;
	(cni->do_change)(0);

	clk_node_set_parent_inner(id);
	clk_node_calc_freq_inner(id);
	return AM18X_OK;
}

uint32_t dev_get_freq(uint32_t dclk_id) {
	uint32_t cn_id = dclk_id / DCLK_ID_GRP_SZ;

	return clk_node_get_freq(cn_id);
}
