/**
*  \file motor.c
*    
*  \brief Driver implementation to control LEGO servo motors. This file contains function definitions to use motors with EV3.
* 
*  LEGO EV3 can be plugged with 4 motors on ports A, B, C and D of the brick. The Enhanced High Resolution Pulse Width Modulator(EHRPWM) of TexasInstruments Sitara AM1808 SoC manages motors speed. There are two ways to stop motors: brake (immediate stop) and float (soft stop). You can get and set rotation angle of motors in degrees anytime. Additionaly you can set a target degree for some motor to reach and stop.
*
*  The identification of motor types (large, medium and nxt) was not implemented. So all speed calculations are configured for nxt motors.
*  
*  \author Bektur Marat uulu and Bektur Toktosunov
*/

/* Include statements */
#include "motor.h"
#include "gpio.h"
#include "adc.h"
#include "hw_syscfg0_AM1808.h"
#include "soc_AM1808.h"
#include "interrupt.h"
#include "evmAM1808.h"
#include "ehrpwm.h"
#include "stdio.h"
#include "psc.h"
#include "hw_gpio.h"
#include "../include/gpio.h"
#include "systick.h"
#include "hw_ecap.h"
#include "hw_tmr.h"

/* Macro definitions */
/**
* \brief Pin Multiplexing bit mask to select EPWM1A pin.
**/
#define PINMUX5_EPWM1A_ENABLE	(SYSCFG_PINMUX5_PINMUX5_3_0_EPWM1A << SYSCFG_PINMUX5_PINMUX5_3_0_SHIFT)

/**
* \brief Pin Multiplexing bit mask to select EPWM1B pin.
**/
#define PINMUX5_EPWM1B_ENABLE	(SYSCFG_PINMUX5_PINMUX5_7_4_EPWM1B << SYSCFG_PINMUX5_PINMUX5_7_4_SHIFT)

/**
* \brief Pin Multiplexing bit mask to select ECAP0 pin.
**/
#define PINMUX2_ECAP0_ENABLE	(SYSCFG_PINMUX2_PINMUX2_31_28_ECAP0 << SYSCFG_PINMUX2_PINMUX2_31_28_SHIFT)

/**
* \brief Multiplexing bit mask to select ECAP1 pin.
**/
#define PINMUX1_ECAP1_ENABLE	(SYSCFG_PINMUX1_PINMUX1_31_28_ECAP1 << SYSCFG_PINMUX1_PINMUX1_31_28_SHIFT)

/**
* \brief Maximal nuber that pwm counts to is 10000. This value was taken from ev3sources.
**/
#define MAX_PWM_CNT (10000)

void ehrpwm1_pin_mux_setup(void);
void int_gpio_enable(void);

/* Global variable definitions */
/**
 * \brief Array storing the required GPIO pins of motors
 */
static motor_port_info ports[] = 
{
    { GPIO_PIN(3, 15), GPIO_PIN(3,  6), GPIO_PIN(5,  4), GPIO_PIN(5, 11), GPIO_PIN(0,  4), (unsigned short) 14 },
    { GPIO_PIN(2,  1), GPIO_PIN(0,  3), GPIO_PIN(2,  5), GPIO_PIN(5,  8), GPIO_PIN(2,  9), (unsigned short) 13 },
    { GPIO_PIN(6,  8), GPIO_PIN(5,  9), GPIO_PIN(3,  8), GPIO_PIN(5, 13), GPIO_PIN(3, 14), (unsigned short) 0 },
    { GPIO_PIN(5,  3), GPIO_PIN(5, 10), GPIO_PIN(5, 15), GPIO_PIN(6,  9), GPIO_PIN(2,  8), (unsigned short) 1 },
};
volatile motor_port_info* addrMotorPorts = ports;

/**
 * \brief Array storing the information about motor speed, current and target revolution counts and brake mode 
 */
motor_data_struct motor_data[4];

/**
 * \brief Array storing the required GPIO pin masks for motors on ports A, B and C.
 */
unsigned int masks[3] = {0x08000000, 0x01000000, 0x20000000};

/**
 * set_duty[0]  = set_duty_mb;
 * set_duty[1]  = set_duty_ma;
 * set_duty[2]  = set_duty_mc;
 * set_duty[3]  = set_duty_md;
 */
static void (*set_duty[NO_OF_OUTPUT_PORTS])(U32) = {set_duty_mb,set_duty_ma,set_duty_mc,set_duty_md};

/**
* \brief - Initialize EHRPWM. 
* 
* Enable EHRPWM and ECAP modules. PWM counts from 0 to counter period (10000). Set compare values of pwm and ecap modules to 0. A duty cicle of a motor begins when PWM counter reaches a given compare value and ends by reaching the counter period value (10000). 
*
* \return none
**/
void init_ehrpwm(void)
{
    SysCfgRegistersUnlock();

    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_EHRPWM, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_ECAP0_1_2, PSC_POWERDOMAIN_ALWAYS_ON, PSC_MDCTL_NEXT_ENABLE);

    ehrpwm1_pin_mux_setup();

    /* Setting baseAddr of EHRPWM1 */
    unsigned int baseAddr = SOC_EHRPWM_1_REGS;

    /* TimeBase configuration */
    /* Setting time base period counter (PwmFrequency) to 10000-1. TBPRD = 10000-1*/
    HWREGH(baseAddr + EHRPWM_TBPRD) = (unsigned short)MAX_PWM_CNT-1; /** for Motor A and Motor B */

    /* Set TBCTL to (TB_UP | TB_DISABLE | TB_SHADOW | TB_SYNC_DISABLE | TB_HDIV1 | TB_DIV1 | TB_COUNT_UP) */
    /* Setting counter direction to up. CTRMODE = TB_UP */
    HWREGH(baseAddr + EHRPWM_TBCTL) = (HWREGH(baseAddr + EHRPWM_TBCTL) & (~EHRPWM_COUNTER_MODE_MASK)) | ((EHRPWM_COUNT_UP <<
        EHRPWM_TBCTL_CTRMODE_SHIFT) &  EHRPWM_COUNTER_MODE_MASK); 

    /* Disable synchronization. PHSEN = TB_DISABLE*/
    EHRPWMTimebaseSyncDisable(SOC_EHRPWM_1_REGS);

    /* Disable shadow write. PRDLD = TB_SHADOW*/
    HWREGH(baseAddr + EHRPWM_TBCTL) = (HWREGH(baseAddr + EHRPWM_TBCTL) & (~EHRPWM_PRD_LOAD_SHADOW_MASK)) | ((EHRPWM_SHADOW_WRITE_DISABLE <<
        EHRPWM_TBCTL_PRDLD_SHIFT) & EHRPWM_PRD_LOAD_SHADOW_MASK);

    /* Disable syncout. SYNCOSEL = TB_SYNC_DISABLE*/
    EHRPWMSyncOutModeSet(SOC_EHRPWM_1_REGS, EHRPWM_SYNCOUT_DISABLE);

    /* Set HSPCLKDIV = TB_DIV1 */
    HWREGH(baseAddr + EHRPWM_TBCTL) = (HWREGH(baseAddr + EHRPWM_TBCTL) &
        (~EHRPWM_TBCTL_HSPCLKDIV)) | ((EHRPWM_TBCTL_HSPCLKDIV_DIVBY1 <<
        EHRPWM_TBCTL_HSPCLKDIV_SHIFT) & EHRPWM_TBCTL_HSPCLKDIV);    

    /* Set CLKDIV = TB_DIV1 */
        HWREGH(baseAddr + EHRPWM_TBCTL) = (HWREGH(baseAddr + EHRPWM_TBCTL) &
        (~EHRPWM_TBCTL_CLKDIV)) | ((EHRPWM_TBCTL_CLKDIV_DIVBY1 <<
        EHRPWM_TBCTL_CLKDIV_SHIFT) & EHRPWM_TBCTL_CLKDIV);


    /* Configure Counter compare sub-module */
    /* Load Compare A value 0 */
    set_duty_ma(0);

    /* Load Compare B value 0 */
    set_duty_mb(0);

    /* Configure CMPCTL to
    SHDWAMODE = CC_SHADOW
    SHDWBMODE = CC_SHADOW
    LOADAMODE = CC_CTR_ZERO 
    LOADBMODE = CC_CTR_ZERO */

    HWREGH(baseAddr + EHRPWM_CMPCTL) = (HWREGH(baseAddr + EHRPWM_CMPCTL) &
        (~EHRPWM_CMPCTL_SHDWAMODE)) | ((EHRPWM_SHADOW_WRITE_ENABLE <<
        EHRPWM_CMPCTL_SHDWAMODE_SHIFT) & EHRPWM_CMPCTL_SHDWAMODE);

    HWREGH(baseAddr + EHRPWM_CMPCTL) = (HWREGH(baseAddr + EHRPWM_CMPCTL)
        & (~EHRPWM_CMPCTL_SHDWBMODE)) | ((EHRPWM_SHADOW_WRITE_ENABLE <<
        EHRPWM_CMPCTL_SHDWBMODE_SHIFT) & EHRPWM_CMPCTL_SHDWBMODE);

    HWREGH(baseAddr + EHRPWM_CMPCTL) = (HWREGH(baseAddr + EHRPWM_CMPCTL) &
        (~EHRPWM_COMPA_LOAD_MASK)) |((EHRPWM_COMPA_LOAD_COUNT_EQUAL_ZERO <<
        EHRPWM_CMPCTL_LOADAMODE_SHIFT) & EHRPWM_COMPA_LOAD_MASK);

    HWREGH(baseAddr + EHRPWM_CMPCTL) = (HWREGH(baseAddr + EHRPWM_CMPCTL) &
        (~EHRPWM_COMPB_LOAD_MASK)) | ((EHRPWM_COMPB_LOAD_COUNT_EQUAL_ZERO <<
        EHRPWM_CMPCTL_LOADBMODE_SHIFT) & EHRPWM_COMPB_LOAD_MASK);


    /* Configure Action qualifier */
    /* Configure AQCTLA */
    EHRPWMConfigureAQActionOnA(SOC_EHRPWM_1_REGS, EHRPWM_AQCTLA_ZRO_EPWMXALOW, EHRPWM_AQCTLA_PRD_DONOTHING,
            EHRPWM_AQCTLA_CAU_EPWMXAHIGH,  EHRPWM_AQCTLA_CAD_DONOTHING,  EHRPWM_AQCTLA_CBU_DONOTHING,
            EHRPWM_AQCTLA_CBD_DONOTHING, EHRPWM_AQSFRC_ACTSFA_DONOTHING);
    
    /* Configure AQCTLB */
    EHRPWMConfigureAQActionOnB(SOC_EHRPWM_1_REGS, EHRPWM_AQCTLB_ZRO_EPWMXBLOW, EHRPWM_AQCTLB_PRD_DONOTHING,
            EHRPWM_AQCTLB_CAU_DONOTHING,  EHRPWM_AQCTLB_CAD_DONOTHING,  EHRPWM_AQCTLB_CBU_EPWMXBHIGH,
            EHRPWM_AQCTLB_CBD_DONOTHING, EHRPWM_AQSFRC_ACTSFB_DONOTHING);

    /* eCAP modules - APWM. Copied from official linux implementation */
    HWREGH(SOC_ECAP_0_REGS + ECAP_TSCTR) = 0;
    HWREGH(SOC_ECAP_1_REGS + ECAP_TSCTR) = 0;
    HWREGH(SOC_ECAP_0_REGS + ECAP_CTRPHS) = 0;
    HWREGH(SOC_ECAP_1_REGS + ECAP_CTRPHS) = 0;
    HWREGH(SOC_ECAP_0_REGS + ECAP_ECCTL2) = 0x0690;
    HWREGH(SOC_ECAP_1_REGS + ECAP_ECCTL2) = 0x0690;
    HWREGH(SOC_TMR_3_REGS + TMR_TGCR) = 0x00003304;
    HWREGH(SOC_TMR_3_REGS + TMR_TGCR) |= 0x00000002;

    /* TimeBase configuration */
    /* Setting time base period counter (PwmFrequency) to 10000-1. TBPRD = 10000-1*/
    HWREGH(SOC_ECAP_0_REGS + ECAP_CAP1) = (unsigned short)MAX_PWM_CNT-1;
    HWREGH(SOC_ECAP_1_REGS + ECAP_CAP1) = (unsigned short)MAX_PWM_CNT-1;
    /* Configure Counter compare sub-module */
    /* Load ecap value 0 */
    set_duty_mc(0);

    /* Load ecap value 0 */
    set_duty_md(0);

    int_gpio_enable();

    SysCfgRegistersLock();
}

/**
* \brief - Load Compare A value
* 
* \param duty - Compare value epwm1A (Port B)
*
* \return none
**/
void set_duty_ma(U32 duty)
{
     unsigned int baseAddr = SOC_EHRPWM_1_REGS;
     HWREGH(baseAddr + EHRPWM_CMPA) = (unsigned short) duty;
     
}

/**
* \brief - Load Compare B value
* 
* \param duty - Compare value of epwm1B (Port A)
*
* \return none
**/
void set_duty_mb(U32 duty)
{
     unsigned int baseAddr = SOC_EHRPWM_1_REGS;
     HWREGH(baseAddr + EHRPWM_CMPB) = (unsigned short) duty;
}

/**
* \brief - Load Compare C value
* 
* \param duty - Compare value of ecap1 (Port C)
*
* \return none
**/
void set_duty_mc(U32 duty)
{
     HWREGH(SOC_ECAP_1_REGS + ECAP_CAP2) = duty & ECAP_CAP2_CAP2;
}

/**
* \brief - Load Compare D value
* 
* \param duty - Compare value of ecap0 (Port D)
*
* \return none
**/
void set_duty_md(U32 duty)
{
     HWREGH(SOC_ECAP_0_REGS + ECAP_CAP2) = duty & ECAP_CAP2_CAP2;
}

/**
* \brief - Set power percent applied to motors. The calculation are appropriated to nxt motors
* 
* \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
* \param power - Power percent from -100 to 100. Rotate backward if a negative number is given.
*
* \return none
**/
void set_power(motor_port_id port, S32 power)
{
    if (100 < power)
    {
        power = 100;
    }
    if (-100 > power)
    {
        power = -100;
    }

    if (0 != power)
    {
        if (0 < power)
        {
            motor_set_state (port, MOTOR_FORWARD);
        }
        else
        {
            motor_set_state (port, MOTOR_BACKWARD);
            power = 0 - power;
        }
        power = (5500-(power * 55))+1;

    } else{
        motor_set_state (port, MOTOR_OFF);
    }

    set_duty[port](power);    
}

/**
 * \brief - This function does appropriate Pin multiplexing to enable the use of
 * 		pwm1, ecap0 and ecap1 related pins on the board.
 *          
 * \return  None.
 */
void ehrpwm1_pin_mux_setup(void)
{
    /* EPWM1A */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) =  
	    	(HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) & (~(SYSCFG_PINMUX5_PINMUX5_3_0))) |
	    	(PINMUX5_EPWM1A_ENABLE);

    /* EPWM1B */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) =  
	    	(HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) & (~(SYSCFG_PINMUX5_PINMUX5_7_4))) |
	    	(PINMUX5_EPWM1B_ENABLE);

    /* Enable PWM Clock in chip config reg 1 */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_CFGCHIP1) |= SYSCFG_CFGCHIP1_TBCLKSYNC;

    /* ecap0 */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(2)) =  
	    	(HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(2)) & (~(SYSCFG_PINMUX2_PINMUX2_31_28))) |
	    	(PINMUX2_ECAP0_ENABLE);

    /* ecap1 */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =  
	    	(HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) & (~(SYSCFG_PINMUX1_PINMUX1_31_28))) |
	    	(PINMUX1_ECAP1_ENABLE);

    /* Enable PWM(ecap) Clock in chip config reg 1 */
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_CFGCHIP1) |= SYSCFG_CFGCHIP1_CAP0SRC;
    HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_CFGCHIP1) |= SYSCFG_CFGCHIP1_CAP1SRC;
}



  /** 
   * \brief This function is not used. For future use.
   *  adc ch.14 -> A
   *  adc ch.13 -> B
   *  adc ch.0  -> C
   *  adc ch.1  -> D
   *  NXT_SERVO_ID (124)
   *  EV3_MEDIUM_MOTOR_ID (288)
   *  EV3_LARGE_MOTOR_ID (3692)
   */
motor_type_id get_motor_type(motor_port_id Port)
{
    gpio_init_outpin(ports[Port].pin6);
    unsigned short vol_val = adc_get((unsigned short) ports[Port].adc);
    if(vol_val > 120 && vol_val < 200){
        return NXT_SERVO_MOTOR;
    } else if(vol_val > 200 && vol_val < 300){
        return EV3_MEDIUM_MOTOR;
    }
    return UNKNOWN_MOTOR_TYPE;
}

/**
* \brief Get motor revolution count in degree
* 
* \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
*
* \return revolution in degree
**/
int ev3_get_count(int motor_port_id){
    if (motor_port_id < NO_OF_OUTPUT_PORTS)
        return motor_data[motor_port_id].current_count;
    else
        return 0;
}

/**
* \brief - Set motor revolution count in degree
* 
* \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
* \param count - Motor revolution count to set
*
* \return none
**/
void
ev3_set_count(int motor_port_id, int count)
{
    if (motor_port_id < NO_OF_OUTPUT_PORTS)
      motor_data[motor_port_id].current_count = count;
}

/**
* \brief Set motor target revolution count, brake mode and speed percent. After reaching the target count the given motor stops.
* 
* \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
* \param cmd - brake mode. true - brake, false - float
* \param target_count - Target revolution count to reach in degree
* \param speed_percent - Speed percent to rotate
*
* \return none
**/
void
ev3_motor_command(U32 motor_port_id, int cmd, int target_count, int speed_percent)
{
    if (motor_port_id < NO_OF_OUTPUT_PORTS) {
      motor_data[motor_port_id].target_count = target_count + motor_data[motor_port_id].current_count;
      motor_data[motor_port_id].brake_mode = cmd;

      if(target_count>0){
       if(speed_percent < 0){
	 speed_percent = -speed_percent;
       }
      } else if(target_count < 0){
       if(speed_percent > 0){
	 speed_percent = -speed_percent;
       }
      } else{
	 speed_percent = 0;
      }
      // TODO: Find out how to use (Options: calculate with ISR:)). motor_data[motor_port_id].speed_percent = speed_percent;

      set_power(motor_port_id, speed_percent);
    }
}

/**
* \brief Set brake mode. Brake - stop immediately, float - soft stop
* 
* \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
* \brake_mode - Brake mode. True - brake, false - float
*
* \return none
**/
void
set_brake_mode(int motor_port_id, int brake_mode)
{
    motor_data[motor_port_id].brake_mode = brake_mode;
}

/**
 * \brief Set the state of an attached motor
 *
 * \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
 * \param state - one of MOTOR_FORWARD, MOTOR_BACKWARD, MOTOR_OFF
 *
 * \return none
 */
void 
motor_set_state (motor_port_id port, motor_state state)
{
    if (state == MOTOR_FORWARD)
    {
        gpio_init_inpin(ports[port].pin1);
        gpio_set(ports[port].pin2, 0);
    }
    else if (state == MOTOR_BACKWARD)
    {
        gpio_set(ports[port].pin1, 0);
        gpio_init_inpin(ports[port].pin2);
    }
    else
    {
	if(motor_data[port].brake_mode){
         gpio_set(ports[port].pin1, 1);
         gpio_set(ports[port].pin2, 1);
	} else {
         gpio_set(ports[port].pin1, 0);
         gpio_set(ports[port].pin2, 0);
	}
    }
}

/**
 * \brief Get GPIO value of pin 6 (DIR). Is used to calculate motor revolution.
 *
 * \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
 *
 * \return Pin 6 value of a given port
 */
unsigned int get_tacho_dir(motor_port_id port)
{
    return gpio_get(ports[port].pin6);
}

/**
 * \brief Get GPIO value of pin 5 (INT). Is used to calculate motor revolution.
 *
 * \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
 *
 * \return Pin 5 value of a given port
 */
unsigned int get_tacho_int(motor_port_id port)
{
    return gpio_get(ports[port].pin5r);
}

/**
 * \brief Initialize the GPIO pins and the pwm modules necessary for motor moving functions
 *
 * \return none
 */
void motor_init (void)
{
    set_duty[0]  = set_duty_mb;
    set_duty[1]  = set_duty_ma;
    set_duty[2]  = set_duty_md;
    set_duty[3]  = set_duty_mc;
    init_ehrpwm();

    unsigned int i;
    for (i = 0; i < sizeof(ports) / sizeof(ports[0]); ++i)
    {
        gpio_init_inpin(ports[i].pin1);
        gpio_init_inpin(ports[i].pin2);
        gpio_init_outpin(ports[i].pin5w);
        gpio_init_inpin(ports[i].pin5r);
        gpio_init_inpin(ports[i].pin6);
    }

    // disable pull-down
    *((volatile unsigned int*)(SYSCFG1_BASE + 0x0C)) &= ~0xFFFFFFFF;
}


/**
 * \brief Interrupt service routine for Pin Bank 5 (ports A, B and C). Calls ev3_motor_quad_decode() if one of the 3 GPIO pins caused this interrupt. Is used to calculate motor revolution in degrees.
 *
 * \return none
 */
void gpioISR5(void) {
    GPIOBankIntDisable(SOC_GPIO_0_REGS, 5);
    unsigned int intstatRegister = HWREG(SOC_GPIO_0_REGS + GPIO_INTSTAT(2));

    // This loop is required since there are 3 GPIO pins on bank 5 that could cause this interrupt
    for (int i = 0; i < 3; ++i) {
        if (intstatRegister & masks[i]) {
            HWREG(SOC_GPIO_0_REGS + GPIO_INTSTAT(2)) = masks[i];
            
            ev3_motor_quad_decode(i);
        }
    }
    IntSystemStatusClear(SYS_INT_GPIOB5);
    
    GPIOBankIntEnable(SOC_GPIO_0_REGS, 5);
}

/**
 * \brief Interrupt service routine for Pin Bank 6. Calls ev3_motor_quad_decode() to actualize current wheel revolution count of motor on port D.
 *
 * \return none
 */
void gpioISR6(void) {
    GPIOBankIntDisable(SOC_GPIO_0_REGS, 6);
    IntSystemStatusClear(SYS_INT_GPIOB6); // AINTC
    HWREG(SOC_GPIO_0_REGS + GPIO_INTSTAT(3)) = HWREG(SOC_GPIO_0_REGS + GPIO_INTSTAT(3)) | 0x00000200; // GPIO-Controller

    ev3_motor_quad_decode(3);
    
    GPIOBankIntEnable(SOC_GPIO_0_REGS, 6);
}

/**
 * \brief Enable GPIO interrupts for bank 5 and 6
 *
 * \return none
 */
void int_gpio_enable(void) {
    // AINTC for GPIO bank 5 and 6 interrupts
    IntRegister(SYS_INT_GPIOB5, gpioISR5);
    IntRegister(SYS_INT_GPIOB6, gpioISR6);
    IntChannelSet(SYS_INT_GPIOB5, 0);
    IntChannelSet(SYS_INT_GPIOB6, 0);
    IntSystemEnable(SYS_INT_GPIOB5);
    IntSystemEnable(SYS_INT_GPIOB6);
    // GPIO-Controller for GPIO bank 5 and 6 interrupts
    unsigned int baseAddr = SOC_GPIO_0_REGS;
    HWREG(baseAddr + GPIO_BINTEN) = HWREG(baseAddr + GPIO_BINTEN) | 0x00000060; // Enable Interrupt for bank 5 and 6 at the same time
    // For bank 5
    HWREG(baseAddr + GPIO_SET_RIS_TRIG(2)) = HWREG(baseAddr + GPIO_SET_RIS_TRIG(2)) | 0x29000000;
    HWREG(baseAddr + GPIO_SET_FAL_TRIG(2)) = HWREG(baseAddr + GPIO_SET_FAL_TRIG(2)) | 0x29000000;
    // For bank 6
    HWREG(baseAddr + GPIO_SET_RIS_TRIG(3)) = HWREG(baseAddr + GPIO_SET_RIS_TRIG(3)) | 0x00000200;
    HWREG(baseAddr + GPIO_SET_FAL_TRIG(3)) = HWREG(baseAddr + GPIO_SET_FAL_TRIG(3)) | 0x00000200;
}


/**
 * \brief Calculate actual revolution count of given motor port.
 *
 * \param motor_port_id - Motor port id (MOTOR_PORT_A, MOTOR_PORT_B, MOTOR_PORT_C, MOTOR_PORT_D)
 * \return none
 */
void
ev3_motor_quad_decode(int motor_port_id)
{

    unsigned int edge = get_tacho_int(motor_port_id);
    unsigned int dir = get_tacho_dir(motor_port_id);

    if (edge != motor_data[motor_port_id].last) {
        if (edge && !dir)
            motor_data[motor_port_id].current_count++;
        else if (edge && dir)
            motor_data[motor_port_id].current_count--;
        else if (!edge && dir)
            motor_data[motor_port_id].current_count++;
        else if (!edge && !dir)
            motor_data[motor_port_id].current_count--;
        motor_data[motor_port_id].last = edge;

        if(motor_data[motor_port_id].current_count==motor_data[motor_port_id].target_count){
            motor_set_state (motor_port_id, MOTOR_OFF);
        }
    }
}
