// tary, 20:47 2017/6/20
#include "am18x_ecap.h"
#include "am18x_dclk.h"

am18x_rt ecap_init(ECAP_con_t* econ) {
	uint32_t v;

	v = econ->ECCTLx[CAPx_ECCTL2];
	v = FIELD_SET(v, ECCTL2_operating_MASK, ECCTL2_operating_APWM);
	v = FIELD_SET(v, ECCTL2_APWMPOL_MASK, ECCTL2_APWMPOL_high);
	v = FIELD_SET(v, ECCTL2_SYNCI_EN_MASK, ECCTL2_SYNCI_EN_no);
	v = FIELD_SET(v, ECCTL2_SYNCO_SEL_MASK, ECCTL2_SYNCO_SEL_disable);
	econ->ECCTLx[CAPx_ECCTL2] = v;

	return AM18X_OK;
}

am18x_rt ecap_get_conf(ECAP_con_t* econ, ecap_conf_t* conf) {
	uint32_t dev_freq, r, cmp;

	dev_freq = dev_get_freq(DCLK_ID_ECAPS);

	// CMP >= PERIOD + 1, output high/low always
	r = econ->CAPx[CAPx_APRD] + 1;
	cmp = econ->CAPx[CAPx_ACMP];

	conf->freq = dev_freq / r;

	if (cmp >= r) {
		conf->duty = 100;
	} else {
		conf->duty = cmp * 100 / r;
	}

	conf->cflags = 0;
	r = econ->ECCTLx[CAPx_ECCTL2];
	if (FIELD_GET(r, ECCTL2_APWMPOL_MASK) == ECCTL2_APWMPOL_high) {
		conf->cflags |= ECAP_ACTIVE_HIGH;
	} else {
		conf->cflags |= ECAP_ACTIVE_LOW;
	}

	if (FIELD_GET(r, ECCTL2_TSCTRSTOP_MASK) == ECCTL2_TSCTRSTOP_no) {
		conf->cflags |= ECAP_ENABLE;
	} else {
		conf->cflags |= ECAP_DISABLE;
	}

	return AM18X_OK;
}

am18x_rt ecap_set_conf(ECAP_con_t* econ, const ecap_conf_t* conf) {
	uint32_t dev_freq, cmp, r, v;

	// stop running
	r = econ->ECCTLx[CAPx_ECCTL2];
	r = FIELD_SET(r, ECCTL2_TSCTRSTOP_MASK, ECCTL2_TSCTRSTOP_yes);
	econ->ECCTLx[CAPx_ECCTL2] = r;

	dev_freq = dev_get_freq(DCLK_ID_ECAPS);

	// 15.2.2.7 Shadow Load and Lockout Control
	// In APWM mode, shadow loading is active and two choices are permitted:
	// * Immediate - APRD or ACMP are transferred to CAP1 or CAP2 immediately
	//   upon writing a new value.
	// * On period equal, CTR[31:0] = PRD[31:0]

	// Note: In APWM mode, writing to CAP1/CAP2 active registers also writes
	//   the same value to the corresponding shadow registers CAP3/CAP4.
	//   This emulates immediate mode.
	//   Writting to the shadow registers CAP3/CAP4 invokes the shadow mode.
	v = dev_freq / conf->freq;
	econ->CAPx[CAPx_APRD] = v - 1;

	if (conf->duty >= 100) {
		cmp = v;
	} else {
		cmp = conf->duty * v / 100;
	}
	econ->CAPx[CAPx_ACMP] = cmp;

	// active high/low?
	r = econ->ECCTLx[CAPx_ECCTL2];
	v = (conf->cflags & ECAP_ACTIVE_HIGH)? ECCTL2_APWMPOL_high: ECCTL2_APWMPOL_low;
	r = FIELD_SET(r, ECCTL2_APWMPOL_MASK, v);

	// econ->ECCTLx[CAPx_ECCTL2] = r;
	// r = econ->ECCTLx[CAPx_ECCTL2];

	// start running ?
	if (conf->cflags & ECAP_ENABLE) {
		r = FIELD_SET(r, ECCTL2_TSCTRSTOP_MASK, ECCTL2_TSCTRSTOP_no);
	}
	econ->ECCTLx[CAPx_ECCTL2] = r;

	return AM18X_OK;
}
