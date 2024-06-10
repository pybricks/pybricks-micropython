// tary, 1:03 2012/12/23
#include "am18x_lib.h"
#include "systick.h"
#include "_uart.h"
#include "auxlib.h"
#include "tps6507x.h"
#include "am1808exp.h"

#define SYSTICK_PERIOD			10/* milli seconds */


static const ddr_conf_t mt46h64m16_6 = {
	.ddr2_not_mddr = AM18X_FALSE,
	.page_size = 10,
	.row_size  = 9,
	.bank_cnt  = 4,

	.freq_ck = 166000000,
	.trefi   = 7812, // ns

	.cl   = 3,
	.trfc = 100,
	.trp  = 18,
	.trcd = 18,

	.twr  = 15,
	.tras = 42,
	.trc  = 60,
	.trrd = 12,

	.twtr = 6,
	.todt = 0,
	.txsnr = 132,
	.trtp = 18,

	.txp   = 2,
	.txsrd = 22,
	.tcke  = 1,

	.trasmax = 70000,
	.pasr = 0,
};

static int sata_100m_clk_enable(am18x_bool on_noff) {
	uint32_t v = on_noff? GPIO_LOW: GPIO_HIGH;

	psc_state_transition(PSC_GPIO, PSC_STATE_ENABLE);

	gpio_set_mux(SATA_100M_CLK_OEn, GPIO_DIR_OUTPUT);
	gpio_set_output1(SATA_100M_CLK_OEn, v);

	// gpio_set_mux(SATA_100M_CLK_OEn, GPIO_DIR_INPUT);
	// printk("SATA_100M_CLK_OEn = %d\n", gpio_get_output1(SATA_100M_CLK_OEn));
	return 0;
}

int low_level_init(void) {
	extern int isr_init(void);
	extern int output_a_char(int);
	int r;

	// output_a_char('S');

	syscfg_kick(AM18X_FALSE);

	isr_init();

	clk_node_init();
	uart_init();

	// ddr_initialize(DDR0, &mt46h64m16_6);

	if (AM18X_OK != (r = systick_init(SYSTICK_PERIOD))) {
		printk("systick_init() error\n");
		return r;
	}

/*	if (AM18X_OK != (r = systick_start())) {
		printk("systick_start() error\n");
		return r;
	}
*/
	// invalid operation ?
	sata_100m_clk_enable(AM18X_FALSE);

#ifdef _M_BOOT_DELAY
	delay(284091 * 3);
#endif

	// tps6507x_conf();

	printk("ARM CLK: %9d Hz\n", dev_get_freq(DCLK_ID_ARM));

	return AM18X_OK;
}
