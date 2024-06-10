// tary, 0:48 2013/3/9
#include "am18x_aintc.h"

#define acon		AINTC

static void null_operation(void) {
	return;
}

am18x_rt aintc_conf(const aintc_conf_t* conf) {
	uint32_t reg, msk;
	int i;

	// all system interrupt for IRQ
	for (i = 0; i < AINTC_ASSIGN_CNT; i++) {
		reg = acon->CMRx[CMRx_X(i)];
		msk = CMRx_MASK(i);
		acon->CMRx[CMRx_X(i)] = __field_xset(reg, msk, 2);

		reg = FIELD_SET(0, XIxR_INDEX_MASK, XIxR_INDEX_VAL(i));
		acon->EIxR[AINTC_IDX_CLR] = reg;

		reg = FIELD_SET(0, XIxR_INDEX_MASK, XIxR_INDEX_VAL(i));
		acon->SIxR[AINTC_IDX_CLR] = reg;
	}
	acon->VBR = conf->isr_addr;
	acon->VSR = VSR_SIZE_VAL(conf->isr_size);
	acon->VNR = (uint32_t)&null_operation;

	msk = GER_ENABLE_MASK;
	acon->GER = FIELD_SET(acon->GER, msk, GER_ENABLE_no);

	return AM18X_OK;
}

am18x_rt aintc_enable(aintc_host_t host) {
	uint32_t msk;

	switch (host) {
	case AINTC_GLOBAL:
		msk = GER_ENABLE_MASK;
		acon->GER = FIELD_SET(acon->GER, msk, GER_ENABLE_yes);
		break;
	case AINTC_HOST_IRQ:
		msk = HIER_IRQ_MASK;
		acon->HIER = FIELD_SET(acon->HIER, msk, HIER_IRQ_enabled);
		break;
	case AINTC_HOST_FIQ:
		msk = HIER_FIQ_MASK;
		acon->HIER = FIELD_SET(acon->HIER, msk, HIER_FIQ_enabled);
		break;
	default:
		return AM18X_ERR;
	}
	return AM18X_OK;
}

am18x_rt aintc_disable(aintc_host_t host) {
	uint32_t msk;

	switch (host) {
	case AINTC_GLOBAL:
		msk = GER_ENABLE_MASK;
		acon->GER = FIELD_SET(acon->GER, msk, GER_ENABLE_no);
		break;
	case AINTC_HOST_IRQ:
		msk = HIER_IRQ_MASK;
		acon->HIER = FIELD_SET(acon->HIER, msk, HIER_IRQ_disabled);
		break;
	case AINTC_HOST_FIQ:
		msk = HIER_FIQ_MASK;
		acon->HIER = FIELD_SET(acon->HIER, msk, HIER_FIQ_disabled);
		break;
	default:
		return AM18X_ERR;
	}
	return AM18X_OK;
}

am18x_rt aintc_sys_enable(AINTC_assign_t assign) {
	uint32_t reg;

	reg = FIELD_SET(0, XIxR_INDEX_MASK, XIxR_INDEX_VAL(assign));
	acon->EIxR[AINTC_IDX_SET] = reg;

	// also for acon->ESRx
	return AM18X_OK;
}

am18x_rt aintc_sys_disable(AINTC_assign_t assign) {
	uint32_t reg;

	reg = FIELD_SET(0, XIxR_INDEX_MASK, XIxR_INDEX_VAL(assign));
	acon->EIxR[AINTC_IDX_CLR] = reg;

	// also for acon->ECRx
	return AM18X_OK;
}

am18x_rt aintc_trigger(AINTC_assign_t assign) {
	/*
	int idx;

	idx = SXXRx_WR_X(assign);
	acon->SRSRx[idx] = SXXRx_WR_VAL(assign);
	*/
	uint32_t reg;

	reg = FIELD_SET(0, XIxR_INDEX_MASK, XIxR_INDEX_VAL(assign));
	acon->SIxR[AINTC_IDX_SET] = reg;
	
	return AM18X_OK;
}

am18x_rt aintc_clear(AINTC_assign_t assign) {
	/*
	int idx;

	idx = SXXRx_WR_X(assign);
	acon->SECRx[idx] = SXXRx_WR_VAL(assign);
	*/
	uint32_t reg;

	reg = FIELD_SET(0, XIxR_INDEX_MASK, XIxR_INDEX_VAL(assign));
	acon->SIxR[AINTC_IDX_CLR] = reg;
	return AM18X_OK;
}

int32_t	aintc_get_active(void) {
	int32_t reg;

	reg = acon->HIPIRx[AINTC_IDX_IRQ];
	if (reg < 0) {
		return AINTC_INVALID_ACTIVE;
	}
	return reg;
}
