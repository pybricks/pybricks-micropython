// tary, 22:38 2013/6/5
#include "am18x_edma.h"
#include "auxlib.h"

static EDMA3CC_rgn_t* region_2_reg(EDMA3CC_con_t* ccon, int region) {
	if (ccon == NULL) {
		return NULL;
	}
	switch (region) {
	case REGION_0:
		return &ccon->Region0;
		break;
	case REGION_1:
		return &ccon->Region1;
		break;
	case REGION_GLOBAL:
	default:
		break;
	}
	return &ccon->Global;
}

static am18x_bool is_qchannel(const edma_conf_t* conf) {
	switch (conf->trigger) {
	case QDMA_AUTO_TRIGGERED:
	//case QDMA_LINK_TRIGGERED:
		break;
	case DMA_EVENT_TRIGGERED:
	case DMA_MANUALLY_TRIGGERED:
	//case DMA_CHAIN_TRIGGERED:
		return AM18X_FALSE;
		break;
	}
	return AM18X_TRUE;
}

am18x_bool is_null_pa_entry(const pa_conf_t* pa) {
	if (pa->a_cnt == 0 && pa->b_cnt == 0 && pa->c_cnt == 0) {
		return AM18X_TRUE;
	}
	return AM18X_FALSE;
}

am18x_bool is_dummy_pa_entry(const pa_conf_t* pa) {
	if ((pa->a_cnt == 0 || pa->b_cnt == 0 || pa->c_cnt == 0)
	&& (pa->a_cnt != 0 || pa->b_cnt != 0 || pa->c_cnt != 0)
	) {
		return AM18X_TRUE;
	}
	return AM18X_FALSE;
}

am18x_rt edma_init(EDMA_con_t* econ, const edma_conf_t* conf) {
	uint32_t v, ch;
	EDMA3CC_con_t* ccon = &econ->CC;
	EDMA3CC_rgn_t* rgn;

	ch = conf->channel;
	if (is_qchannel(conf)) {
		v = ccon->QCHMAPx[ch];
		v = __field_xset(v, QCHMAP_PAENTRY_MASK, conf->pa_conf[0].index);
		v = __field_xset(v, QCHMAP_TRWORD_MASK, conf->tr_word);
		ccon->QCHMAPx[ch] = v;
	}

	rgn = region_2_reg(ccon, conf->region);

	if (conf->region != REGION_GLOBAL) {
		if (is_qchannel(conf)) {
			v = ccon->QRAEx[conf->region];
			v = FIELD_SET(v, QRAE_En_MASK(ch), QRAE_En_allow(ch));
			ccon->QRAEx[conf->region] = v;
		} else {
			v = ccon->DRAEx[DRAE_IDX(conf->region)];
			v = FIELD_SET(v, DRAE_En_MASK(ch), DRAE_En_allow(ch));
			ccon->DRAEx[DRAE_IDX(conf->region)] = v;
		}
	}

	switch(conf->trigger) {
	case DMA_EVENT_TRIGGERED:
	case DMA_MANUALLY_TRIGGERED:
		rgn->EESR = FIELD_SET(0, EExR_En_MASK(ch), EExR_En_set(ch));
		break;
	case QDMA_AUTO_TRIGGERED:
		break;
	//case DMA_CHAIN_TRIGGERED:
	//case QDMA_LINK_TRIGGERED:
	//	break;
	}

	if (econ == EDMA0) {
		if (is_qchannel(conf)) {
			v = ccon->QDMAQNUM;
			v = __field_xset(v, QDMAQNUM_En_MASK(ch), conf->queue);
			ccon->QDMAQNUM = v;
		} else {
			v = ccon->DMAQNUMx[DMAQNUM_IDX(ch)];
			v = __field_xset(v, DMAQNUM_En_MASK(ch), conf->queue);
			ccon->DMAQNUMx[DMAQNUM_IDX(ch)] = v;
		}
	}

	return AM18X_OK;
}

am18x_rt edma_param(EDMA_con_t* econ, const edma_conf_t* conf) {
	EDMA3CC_con_t* ccon = &econ->CC;
	PaRAM_entry_t* pa_regs = ccon->PAEntry;
	PaRAM_entry_t pa_entry[1];
	pa_conf_t* pa;
	uint32_t v;
	int i;

	pa = conf->pa_conf;
	for (i = 0; i < conf->pa_cnt; i++, pa++) {
		v = OPT_SAM_INCR | OPT_DAM_INCR  | OPT_FWID_32b |
		  OPT_ITCCHEN_no | OPT_TCCHEN_no | OPT_ITCINTEN_no | OPT_TCINTEN_no |
		  OPT_STATIC_no | OPT_SYNCDIM_Async | OPT_TCCMODE_Normal;
		v = FIELD_SET(v, OPT_PRIVID_MASK, OPT_PRIVID_X(pa->priv_id));
		v = FIELD_SET(v, OPT_TCC_MASK, OPT_TCC_X(pa->tcc));
		if (pa->flags & FLAG_TRANS_EVT) {
			v = FIELD_SET(v, OPT_ITCCHEN_MASK, OPT_ITCCHEN_yes);
			v = FIELD_SET(v, OPT_TCCHEN_MASK, OPT_TCCHEN_yes);
		}
		if (pa->flags & FLAG_TRANS_INTR) {
			// v = FIELD_SET(v, OPT_ITCINTEN_MASK, OPT_ITCINTEN_yes);
			v = FIELD_SET(v, OPT_TCINTEN_MASK, OPT_TCINTEN_yes);
		}
		if (pa->flags & FLAG_TCC_EARLY) {
			v = FIELD_SET(v, OPT_TCCMODE_MASK, OPT_TCCMODE_Early);
		}
		if (pa->flags & FLAG_LAST_PAENTRY) {
			v = FIELD_SET(v, OPT_STATIC_MASK, OPT_STATIC_yes);
		}
		if (pa->flags & FLAG_SYNCTYPE_AB) {
			v = FIELD_SET(v, OPT_SYNCDIM_MASK, OPT_SYNCDIM_ABsync);
		}
		pa_entry->OPT = v;

		pa_entry->SRC = pa->src;

		v = __field_xset(0, PARAM_ACNT_MASK, pa->a_cnt);
		v = __field_xset(v, PARAM_BCNT_MASK, pa->b_cnt);
		pa_entry->A_B_CNT = v;

		pa_entry->DST = pa->dst;

		v = __field_xset(0, PARAM_SRCBIDX_MASK, pa->src_b_idx);
		v = __field_xset(v, PARAM_DSTBIDX_MASK, pa->dst_b_idx);
		pa_entry->SRC_DST_BIDX = v;	

		v = __field_xset(0, PARAM_LINK_MASK, pa->link);
		if ((pa->flags & FLAG_SYNCTYPE_AB) == 0) {
			v = __field_xset(v, PARAM_BCNTRLD_MASK, pa->b_cnt);
		}
		pa_entry->LINK_BCNTRLD = v;

		v = __field_xset(0, PARAM_SRCCIDX_MASK, pa->src_c_idx);
		v = __field_xset(v, PARAM_DSTCIDX_MASK, pa->dst_c_idx);
		pa_entry->SRC_DST_CIDX = v;

		v = __field_xset(0, PARAM_CCNT_MASK, pa->c_cnt);
		pa_entry->CCNT = v;

		pa_regs[pa->index] = *pa_entry;

		// printk("\n&param0 = 0x%.8X\n", &pa_regs[pa->index]);
		// dump_regs_word("pa_orig", (int)pa_entry, sizeof *pa_entry);
	}
	return AM18X_OK;
}

am18x_rt edma_interrupt(EDMA_con_t* econ, const edma_conf_t* conf) {
	EDMA3CC_con_t* ccon = &econ->CC;
	EDMA3CC_rgn_t* rgn;
	pa_conf_t* pa;
	uint32_t msk;
	int i;

	rgn = region_2_reg(ccon, conf->region);

	pa = conf->pa_conf;
	for (i = 0; i < conf->pa_cnt; i++, pa++) {
		if (pa->flags & FLAG_TRANS_INTR) {
			if (conf->region != REGION_GLOBAL && pa->tcc != conf->channel) {
				uint32_t v;

				v = ccon->DRAEx[DRAE_IDX(conf->region)];
				v = FIELD_SET(v, DRAE_En_MASK(pa->tcc), DRAE_En_allow(pa->tcc));
				ccon->DRAEx[DRAE_IDX(conf->region)] = v;
			}
			msk = IExR_En_MASK(pa->tcc);
			rgn->IESR = FIELD_SET(0, msk, IExR_En_set(pa->tcc));
		}

		if (pa->flags & FLAG_TRANS_EVT) {
			msk = EExR_En_MASK(pa->tcc);
			rgn->EESR = FIELD_SET(0, msk, EExR_En_set(pa->tcc));
		}
	}
	return AM18X_OK;
}

am18x_rt edma_transfer(EDMA_con_t* econ, const edma_conf_t* conf) {
	EDMA3CC_con_t* ccon = &econ->CC;
	PaRAM_entry_t* pa_regs = ccon->PAEntry;
	EDMA3CC_rgn_t* rgn;
	vuint32_t* tr_p;
	uint32_t ch;

	rgn = region_2_reg(ccon, conf->region);

	rgn->SECR = rgn->SER;

	ch = conf->channel;
	if (conf->trigger == QDMA_AUTO_TRIGGERED) {
		rgn->QEESR = FIELD_SET(0, QEExR_En_MASK(ch), QEExR_En_set(ch));

		tr_p = (vuint32_t*)&pa_regs[conf->pa_conf[0].index];
		tr_p += conf->tr_word;
		// trigger the transfer by writing the tr_word in param set
		*tr_p = *tr_p;
	} else if (conf->trigger == DMA_MANUALLY_TRIGGERED) {
		rgn->ESR = FIELD_SET(0, ExR_En_MASK(ch), ExR_En_set(ch));
	}
	return AM18X_OK;
}

am18x_rt edma_completed(EDMA_con_t* econ, const edma_conf_t* conf) {
	EDMA3CC_con_t* ccon = &econ->CC;
	EDMA3CC_rgn_t* rgn;
	pa_conf_t* pa;
	uint32_t msk;
	int i;

	rgn = region_2_reg(ccon, conf->region);

	pa = conf->pa_conf;
	for (i = 0; i < conf->pa_cnt; i++, pa++) {
		#if 0
		if ((pa->flags & FLAG_LAST_PAENTRY) == 0) {
			continue;
		}
		#endif
		if ((pa->flags & FLAG_TRANS_INTR) == 0) {
			continue;
		}
		msk = IxR_En_MASK(pa->tcc);
		if (FIELD_GET(rgn->IPR, msk) == IxR_En_none(pa->tcc)) {
			return AM18X_ERR;
		}
	}

	// clear pending bits in IPR
	rgn->ICR = rgn->IPR;

	return AM18X_OK;
}

am18x_rt edma_status(const EDMA_con_t* econ, edma_stat_t* stat) {
	static int bit_nrs[] = {0, 1, 2, 3, 4, 16, 17, -1};
	static char* inner_status[] = {
		"DMA  event active",
		"QDMA event active",
		"transfer request active",
		"write status active",
		"channel controller active",
		"Queue 0 active",
		"Queue 1 active",
		"none",
	};
	uint32_t ccstat;
	int i;

	ccstat = econ->CC.CCSTAT;
	if (FIELD_GET(ccstat, 0x0003001FUL) == 0) {
		return AM18X_ERR;
	}

	if (stat == NULL) {
		return AM18X_OK;
	}

	stat->comp_actv = __field_xget(ccstat, CCSTAT_COMPACTV_MASK);
	stat->queue_evts[0] = __field_xget(econ->CC.QSTATx[0], QSTAT_NUMVAL_MASK);
	if (econ == EDMA0) {
		stat->queue_evts[1] = __field_xget(econ->CC.QSTATx[1], QSTAT_NUMVAL_MASK);
	} else {
		stat->queue_evts[1] = 0;
	}

	for (i = 0; bit_nrs[i] != -1; i++) {
		if (FIELD_GET(ccstat, BIT(bit_nrs[i])) != 0) {
			stat->status[i] = inner_status[i];
		} else {
			stat->status[i] = "none";
		}
	}

	stat->status[i] = NULL;
	return AM18X_OK;
}
