// tary, 20:35 2015/6/6
#include "am18x_usb.h"
#include "am18x_syscfg.h"
#include "am18x_dclk.h"
#include "am18x_psc.h"
#include "auxlib.h"

#define ucon		USB0

am18x_rt usb0_conf(const usb0_conf_t* conf) {
	uint32_t reg, msk, v;
	uint32_t freq;
	int i;

	// Make sure write access key is initialized
	// prior to accessing any of the BOOTCFG registers
	syscfg_kick(AM18X_FALSE);

	psc_state_transition(PSC_USB0, PSC_STATE_ENABLE);

	// Reset the USB controller
	reg = ucon->CTRLR;
	msk = CTRLR_RESET_MASK;
	ucon->CTRLR = FIELD_SET(reg, msk, CTRLR_RESET_reset);
	while (FIELD_GET(ucon->CTRLR, msk) == CTRLR_RESET_reset);

	freq = dev_get_freq(DCLK_ID_USB2_0_PHY);
	printk("USB0 PHY CLK = %d Hz\n", freq);

	// Configure PHY with the Desired Operation
	syscfg_set_usb0phy(AM18X_TRUE, freq);

	reg = ucon->POWER;
	v = (conf->highspeed)? POWER_HSEN_high: POWER_HSEN_full;
	ucon->POWER = FIELD_SET(reg, POWER_HSEN_MASK, v);

	// Enable PDR2.0 Interrupt
	reg = ucon->CTRLR;
	ucon->CTRLR = FIELD_SET(reg, CTRLR_UINT_MASK, CTRLR_UINT_PDR);

	reg = ucon->INTRRXE;
	msk = v = 0;
	for (i = 1; i < USB0_EP_CNT; i++) {
		msk |= INTRx_EP_MASK(i);
		v |= INTRx_EP_enable(i);
	}
	// Enable All Core Rx Endpoints Interrupts
	ucon->INTRRXE = FIELD_SET(reg, msk, v);

	reg = ucon->INTRTXE;
	msk |= INTRx_EP_MASK(0);
	v |= INTRx_EP_enable(0);
	// Enable All Core Tx Endpoints Interrupts + EP0 Tx/Rx Interrupts
	// spruh82a.pdf, Page 1674
	// Use INTRTX only when in the non-PDR interrupt mode, that is,
	// when handling the interrupt directly from the controller
	ucon->INTRTXE = FIELD_SET(reg, msk, v);

	reg = ucon->INTRUSBE;
	msk = v = 0;
	for (i = INTUSB_SUSPEND; i < INTUSB_CNT; i++) {
		msk |= INTRUSBx_INTUSB_MASK(i);
		v |= INTRUSBx_INTUSB_yes(i);
	}
	// Enable all USB interrupts in MUSBMHDRC
	// spruh82a.pdf, Page 1677
	// Use INTRUSB only when in the non-PDR interrupt mode, that is
	// when handling the interrupt directly from the controller
	ucon->INTRUSBE = reg = FIELD_SET(reg, msk, v);
	printk("Write INTRUSBE = 0x%.8X\n", reg);

	/*
	reg = ucon->CTRLR;
	ucon->CTRLR = FIELD_SET(reg, CTRLR_UINT_MASK, CTRLR_UINT_PDR);
	*/

	/* msk |= INTxR_USBDRVVBUS_MASK | INTxR_VBUSERR_MASK | INTxR_SESSREQ_MASK;
	msk |= INTxR_DISCON_MASK | INTxR_CONN_MASK | INTxR_SOF_MASK;
	msk |= INTxR_BABBLE_MASK | INTxR_RESUME_MASK | INTxR_SUSPEND_MASK;
	v |= INTxR_USBDRVVBUS_yes | INTxR_VBUSERR_yes | INTxR_SESSREQ_yes;
	v |= INTxR_DISCON_yes | INTxR_CONN_yes | INTxR_SOF_yes;
	v |= INTxR_BABBLE_yes | INTxR_RESUME_yes | INTxR_SUSPEND_yes;
	*/
	reg = ucon->INTMSKSETR;
	msk = v = 0;
	for (i = 0; i < USB0_EP_CNT; i++) {
		msk |= INTxR_TXEPn_MASK(i);
		v |= INTxR_TXEPn_yes(i);
		if (i == 0) continue;
		msk |= INTxR_RXEPn_MASK(i);
		v |= INTxR_RXEPn_yes(i);
	}
	for (i = INTUSB_SUSPEND; i < INTUSB_CNT; i++) {
		msk |= INTxR_INTUSB_MASK(i);
		v |= INTxR_INTUSB_yes(i);
	}
	// Enable interrupts in OTG block
	// spruh82a.pdf, Page 1667
	// Other than USB bit field, to make use of INTMSKSETR,
	// the PDR interrupt handler must be enabled.
	ucon->INTMSKSETR = FIELD_SET(reg, msk, v);

	reg = ucon->CTRLR;
	ucon->CTRLR = FIELD_SET(reg, CTRLR_UINT_MASK, CTRLR_UINT_non_PDR);

	printk( "### non_PDR ###\n"
		"INTRTXE =    0x%.8X\n"
		"INTRRXE =    0x%.8X\n"
		"INTRUSBE =   0x%.8X\n"
		"INTMSKSETR = 0x%.8X\n",
		ucon->INTRTXE,
		ucon->INTRRXE,
		ucon->INTRUSBE,
		ucon->INTMSKSETR);

	reg = ucon->CTRLR;
	ucon->CTRLR = FIELD_SET(reg, CTRLR_UINT_MASK, CTRLR_UINT_PDR);

	printk( "### PDR ###\n"
		"INTRTXE =    0x%.8X\n"
		"INTRRXE =    0x%.8X\n"
		"INTRUSBE =   0x%.8X\n"
		"INTMSKSETR = 0x%.8X\n",
		ucon->INTRTXE,
		ucon->INTRRXE,
		ucon->INTRUSBE,
		ucon->INTMSKSETR);

	// Enable SUSPENDM so that suspend can be seen UTMI signal
	reg = ucon->POWER;
	ucon->POWER = FIELD_SET(reg, POWER_SUSPENDM_MASK, POWER_SUSPENDM_entry);

	// Clear all pending interrupts
	ucon->INTCLRR = reg = ucon->INTSRCR;
	#if 0
	reg = ucon->INTRTX | ucon->INTRRX;
	printk( "\nINTSRCR =    0x%.8X\n",
		ucon->INTSRCR);

	printk( "\n&INTRUSBE =  0x%.8X\n",
		&ucon->INTRUSBE);
	#endif

	// Set softconn bit
	reg = ucon->POWER;
	ucon->POWER = FIELD_SET(reg, POWER_SOFTCONN_MASK, POWER_SOFTCONN_yes);

	// Stay here until controller goes in Session
	msk = DEVCTL_SESSION_MASK;
	while (FIELD_GET(ucon->DEVCTL, msk) == DEVCTL_SESSION_none);

	return AM18X_OK;
}

uint32_t usb0_intr_state(void) {
	uint32_t reg;

	ucon->INTCLRR = reg = ucon->INTSRCR;
	return reg;
}

am18x_rt usb0_intr_clear(void) {
	ucon->EOIR = 0x0UL;
	return AM18X_OK;
}
