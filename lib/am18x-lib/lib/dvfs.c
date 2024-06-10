// tary, 14:11 2015/5/9
#include "am18x_lib.h"
#include "tps6507x.h"
#include "_uart.h"
#include "dvfs.h"

// am1808.pdf
// 4.2 Recommanded Operating Conditions
// Operating Frequency
typedef struct {
	uint32_t	freq;
	uint16_t	volt;
} opp_t;

static opp_t opps[] = {
	{F_OSCIN,     1000},
	{100000000UL, 1000},
	{200000000UL, 1100},
	{375000000UL, 1200},
	{456000000UL, 1300},
};

static int abs(int x) {
	return x > 0? x: -x;
}

int dvfs_get_opp(void) {
	int volt;
	uint32_t i;

	if (pll_cmd(PLL0, PLL_CMD_IS_ENABLE, 0) != AM18X_OK) {
		return 0;
	}

	volt = tps6507x_get_output(PWR_TYPE_DCDC3);
	for (i = 1; i < countof(opps); i++) {
		if (abs(volt - opps[i].volt) < 50) {
			break;
		}
	}

	if (i >= countof(opps)) {
		i--;
	}
	return i;
}

// unit, mV
int dvfs_get_volt(int opp) {
	if (opp < 0 || opp >= (int) countof(opps)) {
		return 0;
	}
	return opps[opp].volt;
}

int dvfs_get_freq(int opp) {
	if (opp < 0 || opp >= (int) countof(opps)) {
		return 0;
	}
	return opps[opp].freq;
}

#define _1K		1000UL
#define _1M		1000000UL

int dvfs_set_opp(int opp) {
	pll_conf_t pcf[1];
	int l_opp;

	l_opp = dvfs_get_opp();
	if (opp == l_opp) {
		return 0;
	}

	if (opp == 0) {
		pll_cmd(PLL0, PLL_CMD_POWER_DOWN, 0);
		tps6507x_set_output(PWR_TYPE_DCDC3, opps[opp].volt);
		clk_node_recalc();
		uart_init();
		return 0;
	}

	if (opp > l_opp) {
		tps6507x_set_output(PWR_TYPE_DCDC3, opps[opp].volt);
	}
	pll_get_conf(PLL0, pcf);

	/* pll_freq = F_OSCIN / prediv * pllm / postdiv */
	pcf->prediv = 1UL;
	pcf->postdiv = 1UL;
	pcf->pllm = opps[opp].freq * pcf->postdiv / F_OSCIN * pcf->prediv;

	// am1808.pdf
	// Page 79,
	// The multiplier values must be chosen such that the PLL output
	// frequency is between 300 and 600 MHz
	while (F_OSCIN * pcf->pllm / pcf->prediv < 300UL * _1M) {
		pcf->pllm <<= 1;
		pcf->postdiv <<= 1;
	}
	while (F_OSCIN * pcf->pllm / pcf->prediv > 600UL * _1M) {
		pcf->pllm >>= 1;
		pcf->postdiv >>= 1;
	}
	pll_set_conf(PLL0, pcf);
	if (opp < l_opp) {
		tps6507x_set_output(PWR_TYPE_DCDC3, opps[opp].volt);
	}

	clk_node_recalc();
	uart_init();
	return 0;
}
