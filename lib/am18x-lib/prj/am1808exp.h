// tary, 12:16 2015/10/24
#ifndef __AM1808EXP_H__
#define __AM1808EXP_H__

#include "am18x_lib.h"
#include "tps6507x.h"

/*
// 1015640A_AM1808_SOM-M1_Schematic.pdf
#define TPS65070_POWER_ON	GPIO_BANK2, GPIO_PIN_2
#define TPS65070_PB_OUT		GPIO_BANK2, GPIO_PIN_5
#define M25P64_BUFF_OEn		GPIO_BANK2, GPIO_PIN_6
switch to
*/
// 1015115C_AM1808_SOM-M1_Schematic.pdf
#define TPS65070_POWER_ONn	GPIO_BANK2, GPIO_PIN_2
#define TPS65070_INTn		GPIO_BANK2, GPIO_PIN_3
#define TPS65070_PB_OUT		GPIO_BANK2, GPIO_PIN_5
#define SATA_100M_CLK_OEn	GPIO_BANK2, GPIO_PIN_6

#define MAX_RECEIVER_CNT	0x100
// am1808.pdf
// Page 58, 2.7.27 Supply and Ground
typedef enum {
	PRCV_AC_DUMMY		=	PWR_TYPE_AC * MAX_RECEIVER_CNT,


	PRCV_USB_DUMMY		=	PWR_TYPE_USB * MAX_RECEIVER_CNT,


	PRCV_SYS_DUMMY		=	PWR_TYPE_SYS * MAX_RECEIVER_CNT,


	// 3.3V (power up step 5)
	PRCV_DCDC1_DUMMY	=	PWR_TYPE_DCDC1 * MAX_RECEIVER_CNT,
	PRCV_CPU_USB0_VDDA33,		// USB0 PHY 3.3-V supply
	PRCV_CPU_USB1_VDDA33,		// USB1 PHY 3.3-V supply
	PRCV_PCA9306_2,
	PRCV_M25P64,
	PRCV_SPI1_TRANS_U4_2,
	PRCV_SATA_CLK_U20,
	PRCV_LAN8710A_VDDxA,


	// 3.3V_or_1.8V (power up step 4 or 5)
	PRCV_DCDC2_DUMMY	=	PWR_TYPE_DCDC2 * MAX_RECEIVER_CNT,
	PRCV_CPU_DVDD3318_A,		// 1.8V or 3.3-V dual-voltage LVCMOS I/O supply voltage pins, Group A
	PRCV_CPU_DVDD3318_B,		// 1.8V or 3.3-V dual-voltage LVCMOS I/O supply voltage pins, Group B
	PRCV_CPU_DVDD3318_C,		// 1.8V or 3.3-V dual-voltage LVCMOS I/O supply voltage pins, Group C
	PRCV_PCA9306_1,
	PRCV_TCA6416_1,			// Baseboard, IO EXPANDER
	PRCV_SPI1_TRANS_U4_1,
	PRCV_LAN8710A_VDDIO,


	// 1.2V (power up step 2)
	PRCV_DCDC3_DUMMY	=	PWR_TYPE_DCDC3 * MAX_RECEIVER_CNT,
	PRCV_CPU_CVDD,			// Variable (1.2V - 1.0V) core supply voltage pins


	// 1.8V_LDO (power up step 4)
	PRCV_LDO1_DUMMY		=	PWR_TYPE_LDO1 * MAX_RECEIVER_CNT,
	PRCV_CPU_SATA_VDDR,		// SATA PHY 1.8V internal regulator supply
	PRCV_CPU_USB0_VDDA18,		// USB0 PHY 1.8-V supply input
	PRCV_CPU_USB1_VDDA18,		// USB1 PHY 1.8-V supply input
	PRCV_CPU_DVDD18,		// 1.8V I/O supply voltage pins.
					// DVDD18 must be powered even if all of
					// the DVDD3318_x supplies are operated at 3.3V.
	PRCV_CPU_DDR_DVDD18,		// DDR PHY 1.8V power supply pins
	PRCV_mDDR_VDD,
	PRCV_mDDR_VDDQ,


	// 1.2V_LDO (power up step 3)
	PRCV_LDO2_DUMMY		=	PWR_TYPE_LDO2 * MAX_RECEIVER_CNT,
	PRCV_CPU_PLL0_VDDA,		// PLL analog Vdd(1.2-V filtered supply)
	PRCV_CPU_PLL1_VDDA,		// PLL analog Vdd(1.2-V filtered supply)
	PRCV_CPU_RVDD,			// 1.2 V internal ram supply voltage pins.
	PRCV_CPU_SATA_VDD,		// SATA PHY 1.2V logic supply
	PRCV_CPU_USB_CVDD,		// USB0 and USB1 core logic 1.2-V supply input
	PRCV_LAN8710A_VDDCR,

} power_receiver_t;

#endif//__AM1808EXP_H__
