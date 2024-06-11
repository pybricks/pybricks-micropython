// tary, 22:05 2013/3/6
#include "am18x_timer.h"
#include "am18x_dclk.h"

uint32_t timer_input_freq(TIMER_con_t* tcon) {
	if (tcon == TIMER0) {
		return dev_get_freq(DCLK_ID_TIMER64P0);
	}
	if (tcon == TIMER1) {
		return dev_get_freq(DCLK_ID_TIMER64P1);
	}
	if (tcon == TIMER2) {
		return dev_get_freq(DCLK_ID_TIMER64P2);
	}
	if (tcon == TIMER3) {
		return dev_get_freq(DCLK_ID_TIMER64P3);
	}
	return 0UL;
}

am18x_rt timer_conf(TIMER_con_t* tcon, const timer_conf_t* conf) {
	uint32_t reg, msk, v;

	// 30.2.1.4.1.3 64-Bit Timer Configuration Procedure
	// 1. Select 64-bit mode (TIMMODE in TGCR)
	reg = tcon->TGCR;
	reg = FIELD_SET(reg, TGCR_PLUSEN_MASK, TGCR_PLUSEN_available);
	msk = TGCR_TIMMODE_MASK;
	switch (conf->mode) {
	case TIMER_MODE_32_BIT_UNCHANINED:
		v = TGCR_TIMMODE_unchained;
		break;
	case TIMER_MODE_32_BIT_CHAINED:
		v = TGCR_TIMMODE_chained;
		break;
	case TIMER_MODE_WATCHDOG:
		v = TGCR_TIMMODE_watchdog;
		break;
	case TIMER_MODE_64_BIT:
	default:
		v = TGCR_TIMMODE_64bit;
		break;
	}
	tcon->TGCR = FIELD_SET(reg, msk, v);

	// 2. Remove the timer from reset (TIM12RS and TIM34RS in TGCR)
	reg = tcon->TCR;
	reg = FIELD_SET(reg, TGCR_TIM12RS_MASK, TGCR_TIM12RS_reset);
	tcon->TCR = FIELD_SET(reg, TGCR_TIM34RS_MASK, TGCR_TIM34RS_reset);

	// 3. Select the desired timer period (PRD12 and PRD34)
	tcon->PRDx[0] = conf->period - 1UL;
	tcon->PRDx[1] = 0UL;

	// 4. Enable the timer (ENAMODE12 in TCR)
	reg = tcon->TCR;
	reg = FIELD_SET(reg, TCR_ENAMODE34_MASK, TCR_ENAMODE34_disabled);
	msk = TCR_ENAMODE12_MASK;
	switch (conf->operation) {
	case TIMER_OPER_ONE_TIME:
		v = TCR_ENAMODE12_once;
		break;
	case TIMER_OPER_CONTINUOUS:
		v = TCR_ENAMODE12_enabled;
		break;
	case TIMER_OPER_RELOAD:
		v = TCR_ENAMODE12_reload;
		break;
	default:
		v = TCR_ENAMODE12_disabled;
		break;
	}
	reg = FIELD_SET(reg, msk, v);

	if (conf->source == TIMER_SOURCE_INTERNAL) {
		v = TCR_CLKSRC12_internal;
	} else {
		v = TCR_CLKSRC12_external;
	}
	tcon->TCR = FIELD_SET(reg, TCR_CLKSRC12_MASK, v);

	switch(conf->result) {
	case TIMER_RESULT_DMA_SYNC:
	case TIMER_RESULT_INTERRUPT:
		reg = tcon->INTCTLSTAT;
		msk = INTCTLSTAT_PRDINTEN12_MASK;
		tcon->INTCTLSTAT = FIELD_SET(reg, msk, INTCTLSTAT_PRDINTEN12_enable);
		break;
	case TIMER_RESULT_NONE:
	default:
		reg = tcon->INTCTLSTAT;
		msk = INTCTLSTAT_PRDINTEN12_MASK;
		tcon->INTCTLSTAT = FIELD_SET(reg, msk, INTCTLSTAT_PRDINTEN12_disable);
		break;
	}

	msk = EMUMGT_FREE_MASK;
	tcon->EMUMGT = FIELD_SET(tcon->EMUMGT, msk, EMUMGT_FREE_run);

	tcon->TIMx[0] = 0UL;

	return AM18X_OK;
}

am18x_rt timer_cmd(TIMER_con_t* tcon, timer_cmd_t cmd, uint32_t arg) {
	uint32_t reg, msk;

	switch(cmd) {
/*	case TIMER_CMD_PAUSE:
		msk = TCR_ENAMODE12_MASK;
		tcon->TCR = FIELD_SET(tcon->TCR, msk, TCR_ENAMODE12_disabled);
		break;
	case TIMER_CMD_RESTART:
		msk = TCR_ENAMODE12_MASK;
		tcon->TCR = FIELD_SET(tcon->TCR, msk, TCR_ENAMODE12_enabled);
		break;
*/
	case TIMER_CMD_PAUSE:
	case TIMER_CMD_RESET:
		msk = TGCR_TIM12RS_MASK;
		tcon->TGCR = FIELD_SET(tcon->TGCR, msk, TGCR_TIM12RS_reset);
		break;
	case TIMER_CMD_RESTART:
	case TIMER_CMD_START:
		msk = TGCR_TIM12RS_MASK;
		tcon->TGCR = FIELD_SET(tcon->TGCR, msk, TGCR_TIM12RS_none);
		break;
	case TIMER_CMD_RELOAD:
		tcon->RELx[0] = arg - 1UL;
		tcon->RELx[1] = 0UL;
		break;
	case TIMER_CMD_INTR_CLEAR:
		reg = tcon->INTCTLSTAT;
		msk = INTCTLSTAT_PRDINTSTAT12_MASK;
		tcon->INTCTLSTAT = FIELD_SET(reg, msk, INTCTLSTAT_PRDINTSTAT12_clear);
	default:
		break;
	}

	return AM18X_OK;
}

uint32_t timer_get_count(TIMER_con_t* tcon) {
	return tcon->TIMx[0];
}

//#error TIMER
