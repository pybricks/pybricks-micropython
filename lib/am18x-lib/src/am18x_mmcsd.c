// tary, 20:23 2013/7/10
#include "am18x_dclk.h"
#include "am18x_mmcsd.h"
#include "auxlib.h"

static uint32_t mmcsd_get_freq(const MMCSD_con_t* mcon) {
	uint32_t reg, f;

	reg = mcon->MMCCLK;
	f = dev_get_freq(DCLK_ID_MMC_SDS);
	f /= (2 * (__field_xget(reg, MMCCLK_CLKRT_MASK) + 1));
	if (FIELD_GET(reg, MMCCLK_DIV4_MASK) == MMCCLK_DIV4_div4) {
		f /= 2;
	}
	return f;
}

uint32_t mmcsd_xet_freq(MMCSD_con_t* mcon, uint32_t freq) {
	uint32_t reg, msk, v;

	if (freq == 0x0UL) {
		return mmcsd_get_freq(mcon);
	}

	reg = mcon->MMCCLK;
	msk = MMCCLK_CLKRT_MASK;
	v = dev_get_freq(DCLK_ID_MMC_SDS) / (freq * 2);
	if (FIELD_GET(reg, MMCCLK_DIV4_MASK) == MMCCLK_DIV4_div4) {
		v /= 2;
	}
	if (v == 0) v = 1;
	mcon->MMCCLK = FIELD_SET(reg, msk, MMCCLK_CLKRT_VAL(v));

	if (mmcsd_get_freq(mcon) > freq) {
		v++;
		mcon->MMCCLK = FIELD_SET(reg, msk, MMCCLK_CLKRT_VAL(v));
	}

	return mmcsd_get_freq(mcon);
}

am18x_rt mmcsd_con_init(MMCSD_con_t* mcon, const mmcsd_conf_t* conf) {
	uint32_t reg, msk, v;

	// 1. Place the MMC/SD controller in its reset state
	msk = MMCCTL_CMDRST_MASK | MMCCTL_DATRST_MASK;
	v = MMCCTL_CMDRST_disabled | MMCCTL_DATRST_disabled;
	mcon->MMCCTL = FIELD_SET(0, msk, v);

	// 2. Write the required values to other registers
	// Initialize the MMC Control Register
	reg = mcon->MMCCTL;
	msk = MMCCTL_WIDTH0_MASK | MMCCTL_WIDTH1_MASK;
	v = MMCCTL_WIDTH0_1bit | MMCCTL_WIDTH1_1_4bit;
	mcon->MMCCTL = FIELD_SET(reg, msk, v);

	// Initializing the Clock Controller Regisr
	reg = mcon->MMCCLK;
	msk =  MMCCLK_DIV4_MASK | MMCCLK_CLKEN_MASK;
	v = MMCCLK_DIV4_div2 | MMCCLK_CLKEN_low;
	mcon->MMCCLK = FIELD_SET(reg, msk, v);

	mmcsd_xet_freq(mcon, conf->freq);

	// Initialize the Interrupt Mask Register
	mcon->MMCIM = 0x0UL;

	// Initialize the Time-Out Registers
	v = FIELD_SET(0, MMCTOR_TOD25_16_MASK, MMCTOR_TOD25_16_VAL(conf->timeout_dat));
	v = FIELD_SET(v, MMCTOR_TOR_MASK, MMCTOR_TOR_VAL(conf->timeout_rsp));
	mcon->MMCTOR = v;

	reg = mcon->MMCTOR;
	v = MMCTOD_TOD15_0_VAL(conf->timeout_dat);
	mcon->MMCTOD = FIELD_SET(reg, MMCTOD_TOD15_0_MASK, v);

	// Initialize the Data Block Registers
	mcon->MMCBLEN = MMCSD_BLOCK_SIZE;
	mcon->MMCNBLK = 1UL;

	// 3. release the MMC/SD controller from its reset state
	v = MMCCTL_CMDRST_enabled | MMCCTL_DATRST_enabled;
	mcon->MMCCTL = FIELD_SET(0, msk, v);

	// 4. Enable the MMCSD_CLK pin so that the memory clock is
	// sent to the memory card
	reg = mcon->MMCCLK;
	mcon->MMCCLK = FIELD_SET(reg, MMCCLK_CLKEN_MASK, MMCCLK_CLKEN_enabled);

	return AM18X_OK;
}

am18x_rt mmcsd_send_cmd(MMCSD_con_t* mcon, const mmcsd_cmd_t* cmd) {
	uint32_t reg, msk, v;
	uint32_t idx;

	assert(mcon);
	assert(cmd);
	assert(cmd->index < 0x40);

	idx = cmd->index;

	mcon->MMCCIDX = 0;
	mcon->MMCRSP[0] = 0;
	mcon->MMCRSP[1] = 0;
	mcon->MMCRSP[2] = 0;
	mcon->MMCRSP[3] = 0;

	reg = mcon->MMCCMD;
	// Bug Fix, DCLR_clear is going data shift 2 bytes
	reg = FIELD_SET(reg, MMCCMD_DCLR_MASK, MMCCMD_DCLR_none);

	if (cmd->cflags & MMCSD_CMD_F_BUSY) {
		reg = FIELD_SET(reg, MMCCMD_BSYEXP_MASK, MMCCMD_BSYEXP_expected);
	} else {
		reg = FIELD_SET(reg, MMCCMD_BSYEXP_MASK, MMCCMD_BSYEXP_none);
	}

	msk = MMCCMD_WDATX_MASK;
	if (cmd->cflags & MMCSD_CMD_F_DATA) {
		reg = FIELD_SET(reg, msk, MMCCMD_WDATX_yes);
		reg = FIELD_SET(reg, MMCCMD_STRMTP_MASK, MMCCMD_STRMTP_block);

		if (cmd->cflags & MMCSD_CMD_F_WRITE) {
			reg = FIELD_SET(reg, MMCCMD_DTRW_MASK, MMCCMD_DTRW_write);
		} else {
			reg = FIELD_SET(reg, MMCCMD_DMATRIG_MASK, MMCCMD_DMATRIG_triggered);
			reg = FIELD_SET(reg, MMCCMD_DTRW_MASK, MMCCMD_DTRW_read);
		}
	} else {
		reg = FIELD_SET(reg, msk, MMCCMD_WDATX_no);
	}

	msk = MMCCMD_RSPFMT_MASK;
	v = MMCCMD_RSPFMT_none;
	if (cmd->cflags & MMCSD_CMD_F_RSP) {
		if (cmd->cflags & MMCSD_CMD_F_LONG) {
			v = MMCCMD_RSPFMT_136b;
		} else if (cmd->cflags & MMCSD_CMD_F_CRC) {
			v = MMCCMD_RSPFMT_48bCRC;
		} else {
			v = MMCCMD_RSPFMT_48b;
		}
	}
	reg = FIELD_SET(reg, msk, v);

	reg = FIELD_SET(reg, MMCCMD_CMD_MASK, MMCCMD_CMD_VAL(idx));

	mcon->MMCARGHL = cmd->arg;
	mcon->MMCCMD = reg;

	if (cmd->cflags & MMCSD_CMD_F_DATA) {
		printk("*** MMCCMD = 0x%.8X ***\n", mcon->MMCCMD);
	}

	return AM18X_OK;
}

#define TRACK_SAMPLES    40000
#define TRACK_SAVES    0x400
mmcsd_cmd_state_t mmcsd_cmd_state(const MMCSD_con_t* mcon, am18x_bool need_crc) {
#if 1
	uint32_t reg;

	reg = mcon->MMCST0;
	if (FIELD_GET(reg, MMCST0_RSPDNE_MASK) == MMCST0_RSPDNE_done) {
		return MMCSD_SC_RSP_OK;
	}
	if (FIELD_GET(reg, MMCST0_CRCRS_MASK) == MMCST0_CRCRS_detected) {
		return MMCSD_SC_CRC_ERR;
	}
	if (FIELD_GET(reg, MMCST0_TOUTRS_MASK) == MMCST0_TOUTRS_occurred) {
		return MMCSD_SC_RSP_TOUT;
	}
#else
	{
	uint32_t reg_tracks[TRACK_SAVES];
	int i, n = 0;

	reg_tracks[n++] = mcon->MMCST0;
	for (i = 0; i < TRACK_SAMPLES; i++) {
		if (reg_tracks[n - 1] != (reg_tracks[n] = mcon->MMCST0)) {
			n++;
		}
	}

	printk("%s() \n", __func__);
	for (i = 0; i < n; i++) {
		printk("[%.3d] = 0x%.8X\n", i, reg_tracks[i]);
		if (FIELD_GET(reg_tracks[i], MMCST0_RSPDNE_MASK) == MMCST0_RSPDNE_done
		//|| FIELD_GET(reg_tracks[i], MMCST0_BSYDNE_MASK) == MMCST0_BSYDNE_done
		) {
			return MMCSD_SC_RSP_OK;
		}
		if (FIELD_GET(reg_tracks[i], MMCST0_CRCRS_MASK) == MMCST0_CRCRS_detected) {
			return MMCSD_SC_CRC_ERR;
		}
		if (FIELD_GET(reg_tracks[i], MMCST0_TOUTRS_MASK) == MMCST0_TOUTRS_occurred) {
			return MMCSD_SC_RSP_TOUT;
		}
	}
	}
#endif
	return MMCSD_SC_NONE;
}

mmcsd_dat_state_t mmcsd_busy_state(const MMCSD_con_t* mcon) {
	return MMCSD_SD_OK;
}

mmcsd_dat_state_t mmcsd_rd_state(const MMCSD_con_t* mcon) {
#if 1
	uint32_t reg;

	reg = mcon->MMCST0;
	if (FIELD_GET(reg, MMCST0_CRCRD_MASK) == MMCST0_CRCRD_detected) {
		return MMCSD_SD_CRC_ERR;
	}
	if (FIELD_GET(reg, MMCST0_TOUTRD_MASK) == MMCST0_TOUTRD_occurred) {
		return MMCSD_SD_TOUT;
	}
	if (FIELD_GET(reg, MMCST0_DRRDY_MASK) == MMCST0_DRRDY_ready) {
		return MMCSD_SD_RECVED;
	}
	if (FIELD_GET(reg, MMCST0_DATDNE_MASK) == MMCST0_DATDNE_done) {
		return MMCSD_SD_OK;
	}
#else
	uint32_t reg_tracks[TRACK_SAVES];
	int i, n = 0;

	reg_tracks[n++] = mcon->MMCST0;
	for (i = 0; i < TRACK_SAMPLES; i++) {
		if (reg_tracks[n - 1] != (reg_tracks[n] = mcon->MMCST0)) {
			n++;
		}
	}

	printk("%s() \n", __func__);
	for (i = 0; i < n; i++) {
		printk("[%.3d] = 0x%.8X\n", i, reg_tracks[i]);
	}
	for (i = 0; i < n; i++) {
		if (FIELD_GET(reg_tracks[i], MMCST0_CRCRD_MASK) == MMCST0_CRCRD_detected) {
			return MMCSD_SD_CRC_ERR;
		}
		if (FIELD_GET(reg_tracks[i], MMCST0_TOUTRD_MASK) == MMCST0_TOUTRD_occurred) {
			return MMCSD_SD_TOUT;
		}
		if (FIELD_GET(reg_tracks[i], MMCST0_DRRDY_MASK) == MMCST0_DRRDY_ready) {
			return MMCSD_SD_RECVED;
		}
		if (FIELD_GET(reg_tracks[i], MMCST0_DATDNE_MASK) == MMCST0_DATDNE_done) {
			return MMCSD_SD_OK;
		}
	}
#endif
	return MMCSD_SD_NONE;
}

mmcsd_dat_state_t mmcsd_wr_state(const MMCSD_con_t* mcon) {
#if 1
	uint32_t reg;

	reg = mcon->MMCST0;
	if (FIELD_GET(reg, MMCST0_CRCWR_MASK) == MMCST0_CRCWR_detected) {
		return MMCSD_SD_CRC_ERR;
	}
	if (FIELD_GET(reg, MMCST0_DXRDY_MASK) == MMCST0_DXRDY_ready) {
		return MMCSD_SD_SENT;
	}
	if (FIELD_GET(reg, MMCST0_DATDNE_MASK) == MMCST0_DATDNE_done) {
		return MMCSD_SD_OK;
	}
#else
	uint32_t reg_tracks[TRACK_SAVES];
	int i, n = 0;

	reg_tracks[n++] = mcon->MMCST0;
	for (i = 0; i < TRACK_SAMPLES; i++) {
		if (reg_tracks[n - 1] != (reg_tracks[n] = mcon->MMCST0)) {
			n++;
		}
	}

	printk("%s() \n", __func__);
	for (i = 0; i < n; i++) {
		printk("[%.3d] = 0x%.8X\n", i, reg_tracks[i]);
	}
	for (i = 0; i < n; i++) {
		if (FIELD_GET(reg_tracks[i], MMCST0_CRCWR_MASK) == MMCST0_CRCWR_detected) {
			return MMCSD_SD_CRC_ERR;
		}
		if (FIELD_GET(reg_tracks[i], MMCST0_DXRDY_MASK) == MMCST0_DXRDY_ready) {
			return MMCSD_SD_SENT;
		}
		if (FIELD_GET(reg_tracks[i], MMCST0_DATDNE_MASK) == MMCST0_DATDNE_done) {
			return MMCSD_SD_OK;
		}
	}
#endif
	return MMCSD_SD_NONE;
}

am18x_rt mmcsd_get_resp(const MMCSD_con_t* mcon, mmcsd_resp_type_t type, mmcsd_resp_t* resp) {
	int i;

	assert(mcon);
	assert(resp);

	switch(type) {
	case MMCSD_RESP_LONG:
		for (i = 0; i < 4; i++) {
			resp->v[i] = mcon->MMCRSP[i];
		}
		break;
	case MMCSD_RESP_SHORT:
	default:
		resp->v[0] = mcon->MMCRSP[3];
		break;
	}
	return AM18X_OK;
}

am18x_rt mmcsd_cntl_misc(MMCSD_con_t* mcon, const mmcsd_misc_t* misc) {
	uint32_t reg, msk, v;

	if (misc->mflags & MMCSD_MISC_F_BUS4BIT) {
		// Initialize the MMC Control Register
		reg = mcon->MMCCTL;
		msk = MMCCTL_WIDTH0_MASK | MMCCTL_WIDTH1_MASK;
		v = MMCCTL_WIDTH0_4bit | MMCCTL_WIDTH1_1_4bit;
		mcon->MMCCTL = FIELD_SET(reg, msk, v);
		return 0;
	}

	// setting block count
	if (misc->blkcnt) {
		mcon->MMCNBLK = misc->blkcnt;
	}

	// 7. Set the FIFO direction to transmit/receive
	reg = 0;
	msk = MMCFIFOCTL_FIFODIR_MASK;
	if (misc->mflags & MMCSD_MISC_F_WRITE) {
		v = MMCFIFOCTL_FIFODIR_write;
	} else {
		v = MMCFIFOCTL_FIFODIR_read;
	}
	reg = FIELD_SET(reg, msk, v);

	// 8. Set the access width
	msk = MMCFIFOCTL_ACCWD_MASK;
	reg = FIELD_SET(reg, msk, MMCFIFOCTL_ACCWD_4bytes);

	// 9. Set the FIFO threshold
	msk = MMCFIFOCTL_FIFOLEV_MASK;
	if (misc->mflags & MMCSD_MISC_F_FIFO_64B) {
		reg = FIELD_SET(reg, msk, MMCFIFOCTL_FIFOLEV_64B);
	} else {
		reg = FIELD_SET(reg, msk, MMCFIFOCTL_FIFOLEV_32B);
	}

	// 6. Reset the FIFO
	// reg = mcon->MMCFIFOCTL;
	msk = MMCFIFOCTL_FIFORST_MASK;
	if (misc->mflags & MMCSD_MISC_F_FIFO_RST) {
		mcon->MMCFIFOCTL = FIELD_SET(reg, msk, MMCFIFOCTL_FIFORST_reset);
	}

	mcon->MMCFIFOCTL = reg;

	printk("*** MMCFIFOCTL = 0x%.8X ***\n", mcon->MMCFIFOCTL);

	reg = mcon->MMCIM;
	msk = MMCIM_ECRCRS_MASK | MMCIM_ETOUTRS_MASK | MMCIM_ERSPDNE_MASK | MMCIM_EDATDNE_MASK;
	v = MMCIM_ECRCRS_enabled | MMCIM_ETOUTRS_enabled | MMCIM_ERSPDNE_enabled | MMCIM_EDATDNE_enabled;
	reg = FIELD_SET(reg, msk, v);
	if (misc->mflags & MMCSD_MISC_F_WRITE) {
		// 10. Enable the DXRDYINT interrupt
		msk = MMCIM_EDXRDY_MASK | MMCIM_ECRCWR_MASK;
		v = MMCIM_EDXRDY_enabled | MMCIM_ECRCWR_enabled;
	} else {
		// 11. Enable the DRRDYINT interrupt
		msk = MMCIM_EDRRDY_MASK | MMCIM_ECRCRD_MASK | MMCIM_ETOUTRD_MASK;
		v = MMCIM_EDRRDY_enabled | MMCIM_ECRCRD_enabled | MMCIM_ETOUTRD_enabled;
	}
	mcon->MMCIM = FIELD_SET(reg, msk, v);

	return AM18X_OK;
}

uint32_t mmcsd_read(const MMCSD_con_t* mcon) {
	return mcon->MMCDRR;
}

am18x_rt mmcsd_write(MMCSD_con_t* mcon, uint32_t data) {
	mcon->MMCDXR = data;
	return AM18X_OK;
}
