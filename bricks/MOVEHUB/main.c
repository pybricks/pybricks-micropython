#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"

#include "stm32f070xb.h"

#include "accel.h"
#include "adc.h"
#include "button.h"
#include "gpio.h"
#include "led.h"
#include "motor.h"
#include "uartadr.h"

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[8 * 1024];
#endif

int main(int argc, char **argv) {
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    button_init();
    led_init();
    adc_init();
    accel_init();
    motor_init();

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif

soft_reset:
    led_set_rgb(0, 255, 0);

    mp_init();
    #if MICROPY_ENABLE_COMPILER
    #if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
    #else
    pyexec_friendly_repl();
    #endif
    #else
    pyexec_frozen_module("frozentest.py");
    #endif
    mp_deinit();

    for (int p = MOTOR_PORT_A; p <= MOTOR_PORT_D; p++) {
        motor_stop(p, MOTOR_STOP_COAST);
    }
    
    goto soft_reset;

    // TODO: do we really need to deinit hardware on hard reset or power off?

    motor_deinit();
    accel_deinit();
    adc_deinit();
    led_deinit();
    button_deinit();

    return 0;
}

// defined in linker script
extern uint32_t _estack;
// defined in ports/stm32/gchelper_m0.s
uintptr_t gc_helper_get_regs_and_sp(uintptr_t *regs);

void gc_collect(void) {
    // start the GC
    gc_collect_start();

    // get the registers and the sp
    uintptr_t regs[10];
    uintptr_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live on the stack in this function)
    #if MICROPY_PY_THREAD
    gc_collect_root((void**)sp, ((uint32_t)MP_STATE_THREAD(stack_top) - sp) / sizeof(uint32_t));
    #else
    gc_collect_root((void**)sp, ((uint32_t)&_estack - sp) / sizeof(uint32_t));
    #endif

    // trace root pointers from any threads
    #if MICROPY_PY_THREAD
    mp_thread_gc_others();
    #endif

    // end the GC
    gc_collect_end();

    // for debug during development
    gc_dump_info();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
    while (1);
}

void NORETURN __fatal_error(const char *msg) {
    while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif

// this seem to be missing from the header file
#ifndef RCC_CFGR3_ADCSW
#define RCC_CFGR3_ADCSW (1 << 8)
#endif

// special memory addresses defined in linker script
extern uint32_t _fw_isr_vector_src[47];
extern uint32_t _fw_isr_vector_dst[47];

// Called from assembly code in startup routine
void SystemInit(void) {
    // setup the system clock
    // this section mostly copied from ports/stm32/system_stm32f0.c and
    // confirmed with LEGO firmware
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

    // Enable all of the hardware modules we are using

    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN
                |  RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOFEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM15EN | RCC_APB2ENR_SYSCFGCOMPEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART4EN | RCC_APB1ENR_USART3EN;


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

    // enable UART at 115200 baud on BOOST OUT C and D, pin 5 and 6

    // USART 3, BOOST i/o D
    gpio_init(GPIOB, 0, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    gpio_low(GPIOB, 0);
    gpio_init(GPIOC, 4, GPIO_MODE_ALT, GPIO_PULL_NONE, 1); // USART3_TX
    gpio_init(GPIOC, 5, GPIO_MODE_ALT, GPIO_PULL_NONE, 1); // USART3_RX

    // USART 4, BOOST i/o C
    gpio_init(GPIOB, 4, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    gpio_low(GPIOB, 4);
    gpio_init(GPIOC, 10, GPIO_MODE_ALT, GPIO_PULL_NONE, 0); // USART4_TX
    gpio_init(GPIOC, 11, GPIO_MODE_ALT, GPIO_PULL_NONE, 0); // USART4_RX

    USART_REPL->BRR = 48000000 / 115200;
    USART_REPL->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}
