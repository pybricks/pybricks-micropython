// tary, 0:35 2012/12/27
#include "am18x_psc.h"
#include "auxlib.h"

static PSC_con_t* module_to_psc(psc_module_t module, uint32_t* nr) {
	if (module < MODULE_NR_PER_PSC) {
		*nr = module;
		return PSC0;
	}
	*nr = module - MODULE_NR_PER_PSC;
	return PSC1;
}

// 8.3.2 Module State Transitions
am18x_rt psc_state_transition(psc_module_t module, psc_state_t state) {
	PSC_con_t* psc;
	uint32_t nr;
	uint32_t reg, val;

	psc = module_to_psc(module, &nr);

	// 1. Wait for the GOSTAT[x] bit in PTSTAT to clear to 0
	while (FIELD_GET(psc->PTSTAT, PTSTAT_GO0_MASK) != PTSTAT_GO0_no);

	// 2. Set the NEXT bit MDCTLn to 0..5
	switch(state) {
	case PSC_STATE_SW_RST_DISABLE:
		val = MDCTLx_STATE_SwRstDisable;
		break;
	case PSC_STATE_SYNC_RESET:
		val = MDCTLx_STATE_SyncReset;
		break;
	case PSC_STATE_ENABLE:
		val = MDCTLx_STATE_Enable;
		break;
	case PSC_STATE_DISABLE:
	default:
		val = MDCTLx_STATE_Disable;
		break;
	}
	reg = psc->MDCTLx[nr];
	psc->MDCTLx[nr] = FIELD_SET(reg, MDCTLx_STATE_MASK, val);

	// 3. Set the GO[x] bit in PTCMD to 1 to initiate the transition(s)
	psc->PTCMD = FIELD_SET(psc->PTCMD, PTCMD_GO0, PTCMD_GO0);

	// 4. Wait for the GOSTAT[x] bit in PTSTAT to clear to 0
	while (FIELD_GET(psc->PTSTAT, PTSTAT_GO0_MASK) != PTSTAT_GO0_no);

	// while (__field_xget(psc->MDSTATx[nr], MDSTATx_STATE_MASK) != val);

	return AM18X_OK;
}

am18x_rt psc_enable_interrupt(psc_module_t module) {
	PSC_con_t* psc;
	uint32_t nr;
	uint32_t reg, msk, val;

	psc = module_to_psc(module, &nr);

	// 1. Set the EMUIHBIE bit in PDCTLn,
	reg = psc->PDCTL0;
	val = PDCTLx_EMUIHBIE_enable;
	psc->PDCTL0 = FIELD_SET(reg, PDCTLx_EMUIHBIE_MASK, val);
	// the EMUIHBIE and the EMURSTIE bits in MDCTLn
	// to enable the interrupt events that you want.
	reg = psc->MDCTLx[nr];
	msk = MDCTLx_EMUIHBIE_MASK | MDCTLx_EMURSTIE_MASK;
	val = MDCTLx_EMUIHBIE_enable | MDCTLx_EMURSTIE_enable;
	psc->MDCTLx[nr] = FIELD_SET(reg, msk, val);

	// 2. Enable the power sleep controller interrupt (PSCn_ALLINT)
	// in the device interrupt controller.
	return AM18X_OK;
}

psc_state_t psc_get_state(psc_module_t module) {
	PSC_con_t* psc;
	uint32_t nr;
	uint32_t val;
	psc_state_t s = PSC_STATE_DISABLE;

	psc = module_to_psc(module, &nr);

	val = __field_xget(psc->MDSTATx[nr], MDSTATx_STATE_MASK);
	if (MDSTATx_STATE_in_transition(val)) {
		return PSC_STATE_IN_TRANSITION;
	}
	switch (val) {
	case MDSTATx_STATE_SwRstDisable:
		s = PSC_STATE_SW_RST_DISABLE;
		break;
	case MDSTATx_STATE_SyncReset:
		s = PSC_STATE_SYNC_RESET;
		break;
	case MDSTATx_STATE_Disable:
		s = PSC_STATE_DISABLE;
		break;
	case MDSTATx_STATE_Enable:
		s = PSC_STATE_ENABLE;
		break;
	}

	return s;
}

#define KOFP(x)		KOF(PSC_con_t, x)
static kv_t of_regs[] = {
	KOFP(REVID),
	KOFP(INTEVAL),
	KOFP(MERRPR0),
	KOFP(MERRCR0),
	KOFP(PERRPR),
	KOFP(PERRCR),
	KOFP(PTCMD),
	KOFP(PTSTAT),
	KOFP(PDSTAT0),
	KOFP(PDSTAT1),
	KOFP(PDCTL0),
	KOFP(PDCTL1),
	KOFP(PDCFG0),
	KOFP(PDCFG1),
	KOFP(MDSTATx),
	KOFP(MDCTLx),
};

am18x_rt psc_dump_regs(PSC_con_t* pcon) {
	uint32_t* ptr;
	int i, s;

	for (i = 0; i < countof(of_regs) - 2; i++) {
		int of;

		of = of_regs[i].key;
		ptr = (uint32_t*)pcon + (of >> 2);
		printk("%-8s[0x%.8X] = 0x%.8X\n", of_regs[i].val, ptr, *ptr);
	}

	s = MODULE_NR_PER_PSC;
	if (pcon == PSC0) s = s / 2;
	ptr = &pcon->MDSTATx;
	for (i = 0; i < s; i++) {
		printk("MDSTAT%-2d[0x%.8X] = 0x%.8X\n", i, ptr + i, ptr[i]);
	}
	ptr = &pcon->MDCTLx;
	for (i = 0; i < s; i++) {
		printk("MDCTL%-3d[0x%.8X] = 0x%.8X\n", i, ptr + i, ptr[i]);
	}
	return AM18X_OK;
}
