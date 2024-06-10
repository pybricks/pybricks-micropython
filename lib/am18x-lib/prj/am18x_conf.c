// tary, 1:04 2012/12/23
#include "am18x_lib.h"
#include "auxlib.h"

extern int output_a_char(int);

static none_arg_handler_t isr_vector[] = {
	// 0
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 10
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 20
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 30
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 40
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 50
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 60
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 70
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 80
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 90
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	// 100
	NULL,
};

int isr_set_handler(int intr_nr, none_arg_handler_t handle) {
	if (intr_nr >= countof(isr_vector)) {
		return -1;
	}
	isr_vector[intr_nr] = handle;
	return 0;
}

int c_irq_handler(void) {
	int32_t irq_nr = aintc_get_active();

	if (irq_nr == AINTC_INVALID_ACTIVE) {
		return -1;
	}
	if (isr_vector[irq_nr] != NULL) {
		(*isr_vector[irq_nr])();
	}

	aintc_clear(irq_nr);

	return 0;
}

int isr_init(void) {
	aintc_conf_t aconf[1];

	aconf->isr_addr = (uint32_t)isr_vector;
	aconf->isr_size = sizeof isr_vector;

	aintc_conf(aconf);

	aintc_enable(AINTC_GLOBAL);
	aintc_enable(AINTC_HOST_IRQ);

	return 0;
}
