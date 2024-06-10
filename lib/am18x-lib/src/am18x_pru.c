// tary, 22:52 2013/5/10
#include "am18x_pru.h"
#include "auxlib.h"

am18x_rt pru_load(PRU_con_t* pcon, const uint32_t* inst, uint32_t count) {
	uint32_t* pram;
	int i;

	pram = (uint32_t*)PRU_InstRAM0_BASE;
	if (pcon == PRU1) {
		pram = (uint32_t*)PRU_InstRAM1_BASE;
	}

	if (count * sizeof(uint32_t) > PRU_InstRAM0_SIZE) {
		count = PRU_InstRAM0_SIZE / sizeof(uint32_t);
	}

	for (i = 0; i < count; i++) {
		pram[i] = inst[i];
	}
	return AM18X_OK;
}

am18x_rt pru_cmd(PRU_con_t* pcon, pru_cmd_t cmd, uint32_t arg) {
	uint32_t reg, msk;

	reg = pcon->CONTROL;

	switch (cmd) {
	case PRU_CMD_DISABLE:
		msk = CONTROL_COUNTENABLE_MASK;
		pcon->CONTROL = FIELD_SET(reg, msk, CONTROL_COUNTENABLE_no);
		reg = pcon->CONTROL;
		msk = CONTROL_ENABLE_MASK;
		pcon->CONTROL = FIELD_SET(reg, msk, CONTROL_ENABLE_no);
		reg = pcon->CONTROL;
		msk = CONTROL_SOFTRESET_MASK;
		pcon->CONTROL = FIELD_SET(reg, msk, CONTROL_SOFTRESET_yes);
		break;

	case PRU_CMD_ENABLE:
		msk = CONTROL_SOFTRESET_MASK;
		pcon->CONTROL = FIELD_SET(reg, msk, CONTROL_SOFTRESET_yes);
		break;

	case PRU_CMD_RUN:
		msk = CONTROL_COUNTENABLE_MASK;
		pcon->CONTROL = FIELD_SET(reg, msk, CONTROL_COUNTENABLE_yes);
		reg = pcon->CONTROL;
		msk = CONTROL_ENABLE_MASK;
		pcon->CONTROL = FIELD_SET(reg, msk, CONTROL_ENABLE_yes);
		break;

	case PRU_CMD_IS_HALT:
		msk = CONTROL_RUNSTATE_MASK;
		if (FIELD_GET(reg, msk) == CONTROL_RUNSTATE_running) {
			return AM18X_ERR;
		}
		break;

	default:
		break;
	}
	return AM18X_OK;
}

#define KOFP(x)		KOF(PRU_con_t, x)
static kv_t of_regs[] = {
	KOFP(CONTROL),
	KOFP(STATUS),
	KOFP(WAKEUP),
	KOFP(CYCLECNT),
	KOFP(STALLCNT),
	KOFP(CONTABBLKIDX0),
	KOFP(CONTABPROPTR),
	KOFP(INTGPR),
	KOFP(INTCTER),
};

am18x_rt pru_dump_regs(PRU_con_t* pcon) {
	uint32_t* ptr;
	int i;

	for (i = 0; i < countof(of_regs); i++) {
		int of;

		of = of_regs[i].key;
		ptr = (uint32_t*)pcon + (of >> 2);
		printk("%-14s[0x%.8X] = 0x%.8X\n", of_regs[i].val, ptr, *ptr);
	}

	return AM18X_OK;
}
