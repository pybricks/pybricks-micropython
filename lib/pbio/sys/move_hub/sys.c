
#include <string.h>

#include <pbdrv/config.h>
#include <pbdrv/light.h>
#include <pbdrv/motor.h>
#include <pbdrv/time.h>

#include <pbio/button.h>
#include <pbio/light.h>

#include "stm32f070xb.h"

// Bootloader reads this address to know if firmware loader should run
uint32_t bootloader_magic_addr __attribute__((section (".magic")));
#define BOOTLOADER_MAGIC_VALUE  0xAAAAAAAA

static bool button_pressed;
static uint16_t button_press_start_time;

void pbsys_prepare_user_program(void) {
    pbio_light_set_user_mode(true);
}

void pbsys_unprepare_user_program(void) {
    pbio_light_set_user_mode(false);

    // TODO: this should probably call the higher-level pbio_dcmotor_coast() function
    for (pbio_port_t p = PBDRV_CONFIG_FIRST_MOTOR_PORT; p <= PBDRV_CONFIG_LAST_MOTOR_PORT; p++) {
        pbdrv_motor_coast(p);
    }
}

void pbsys_reboot(bool fw_update) {
    if (fw_update) {
        bootloader_magic_addr = BOOTLOADER_MAGIC_VALUE;
    }
    // this function never returns
    NVIC_SystemReset();
}

void pbsys_power_off(void) {
    int i;

    // blink pattern like LEGO firmware
    for (i = 0; i < 3; i++) {
        pbdrv_light_set_color(PBIO_PORT_SELF, 255, 140, 60);
        pbdrv_light_set_pattern(PBIO_PORT_SELF, PBDRV_LIGHT_PATTERN_ON);
        pbdrv_time_delay_usec(50000);
        pbdrv_light_set_pattern(PBIO_PORT_SELF, PBDRV_LIGHT_PATTERN_OFF);
        pbdrv_time_delay_usec(30000);
    }

    // PWM doesn't work while IRQs are disabled? so this needs to be after
    __disable_irq();

    // need to loop because power will stay on as long as button is pressed
    while (true) {
        // setting PB11 low cuts the power
        GPIOB->BRR = GPIO_BRR_BR_11;
    }
}

void pbsys_poll(void) {
    uint16_t now;
    pbio_button_flags_t btn;

    now = pbdrv_time_get_msec();
    pbio_button_is_pressed(PBIO_PORT_SELF, &btn);

    if (btn & PBIO_BUTTON_CENTER) {
        if (button_pressed) {
            // TODO: blink light like LEGO firmware

            // if the button is held down for 5 seconds, power off
            if (now - button_press_start_time > 5000) {
                pbdrv_light_set_pattern(PBIO_PORT_SELF, PBDRV_LIGHT_PATTERN_OFF);
                pbdrv_time_delay_usec(580000);
                pbsys_power_off();
            }
        }
        else {
            button_press_start_time = now;
            button_pressed = true;
        }
    }
    else {
        button_pressed = false;
    }
    // TODO monitor for low battery
}

// this seem to be missing from the header file
#ifndef RCC_CFGR3_ADCSW
#define RCC_CFGR3_ADCSW (1 << 8)
#endif

// special memory addresses defined in linker script
extern uint32_t _fw_isr_vector_src[48];
extern uint32_t _fw_isr_vector_dst[48];

// Called from assembly code in startup_stm32f070xb.s
// this function is a mash up of ports/stm32/system_stm32f0.c from MicroPython
// and the official LEGO firmware
void SystemInit(void) {
    // setup the system clock
    RCC->CR |= RCC_CR_HSION;
    RCC->CFGR = 0; // reset all
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
    RCC->CR &= ~RCC_CR_HSEBYP;
    RCC->CFGR2 &= ~RCC_CFGR2_PREDIV;
    RCC->CFGR3 &= ~(RCC_CFGR3_USART1SW | RCC_CFGR3_I2C1SW | RCC_CFGR3_USBSW | RCC_CFGR3_ADCSW);

    // Reset HSI14 bit
    RCC->CR2 &= ~RCC_CR2_HSI14ON;

    // Disable all interrupts
    RCC->CIR = 0;

    // dpgeorge: enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    // Set flash latency to 1 because SYSCLK > 24MHz
    FLASH->ACR = (FLASH->ACR & ~0x7) | FLASH_ACR_PRFTBE | 0x1; // TODO: FLASH_ACR_LATENCY_Msk is wrong

    // using PLL as system clock
    RCC->CFGR |= RCC_CFGR_PLLMUL12;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
        // wait for PLL to lock
    }

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | (2 << RCC_CFGR_SW_Pos);
    while (((RCC->CFGR >> RCC_CFGR_SWS_Pos) & 0x3) != 2) {
        // Wait for SYSCLK source to change
    }

    // Enable all of the shared hardware modules we are using

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN
                |  RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOFEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_SYSCFGCOMPEN;


    // Keep BOOST alive
    GPIOB->BSRR = GPIO_BSRR_BS_11;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER11_Msk) | (1 << GPIO_MODER_MODER11_Pos);

    // not sure what the rest of these pins do

    // PB6 output, high
    GPIOB->BSRR = GPIO_BSRR_BS_6;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER6_Msk) | (1 << GPIO_MODER_MODER6_Pos);

    // PF0 output, high
    GPIOF->BSRR = GPIO_BSRR_BS_0;
    GPIOF->MODER = (GPIOF->MODER & ~GPIO_MODER_MODER0_Msk) | (1 << GPIO_MODER_MODER0_Pos);

    // PA15 output, high
    GPIOA->BSRR = GPIO_BSRR_BS_15;
    GPIOA->MODER = (GPIOA->MODER & ~GPIO_MODER_MODER15_Msk) | (1 << GPIO_MODER_MODER15_Pos);

    // PB5 output, high
    GPIOB->BSRR = GPIO_BSRR_BS_5;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER5_Msk) | (1 << GPIO_MODER_MODER5_Pos);

    // PC12 output, high
    GPIOC->BSRR = GPIO_BSRR_BS_12;
    GPIOC->MODER = (GPIOC->MODER & ~GPIO_MODER_MODER12_Msk) | (1 << GPIO_MODER_MODER12_Pos);

    // PD2 output, high
    GPIOD->BSRR = GPIO_BSRR_BS_2;
    GPIOD->MODER = (GPIOD->MODER & ~GPIO_MODER_MODER2_Msk) | (1 << GPIO_MODER_MODER2_Pos);

    // PF1 output, high
    GPIOF->BSRR = GPIO_BSRR_BS_1;
    GPIOF->MODER = (GPIOF->MODER & ~GPIO_MODER_MODER1_Msk) | (1 << GPIO_MODER_MODER1_Pos);

    // since the firmware starts at 0x08005000, we need to relocate the
    // interrupt vector table to a place where the CPU knows about it.
    // The space at the start of SRAM is reserved in via the linker script.
    memcpy(_fw_isr_vector_dst, _fw_isr_vector_src, sizeof(_fw_isr_vector_dst));

    // this maps SRAM to 0x00000000
    SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE_Msk) | (3 << SYSCFG_CFGR1_MEM_MODE_Pos);

    // SysTick set for 1ms ticks
    SysTick_Config(PBDRV_CONFIG_SYS_CLOCK_RATE / 1000);
}
