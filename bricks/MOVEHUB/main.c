#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"

#include "stm32f030xc.h"

#include "gpio.h"
#include "uartadr.h"

#if MICROPY_ENABLE_COMPILER
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#endif

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[2048];
#endif

int main(int argc, char **argv) {
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
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
    //do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
    //do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
    #else
    pyexec_frozen_module("frozentest.py");
    #endif
    mp_deinit();
    return 0;
}

void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
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

// these seem to be missing from the header file
#ifndef RCC_CFGR3_USBSW
#define RCC_CFGR3_USBSW (1 << 7)
#endif
#ifndef RCC_CFGR3_ADCSW
#define RCC_CFGR3_ADCSW (1 << 8)
#endif

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

    // enable GPIO clocks
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    RCC->AHBENR |= RCC_AHBENR_GPIODEN;
    RCC->AHBENR |= RCC_AHBENR_GPIOFEN;


    // Keep BOOST alive
    gpio_init(GPIOB, 11, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    gpio_high(GPIOB, 11);

    // // Turn on BLUE LED
    // gpio_init(GPIOB, 15, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    // gpio_high(GPIOB, 15);

    // Turn on GREEN LED
    gpio_init(GPIOB, 14, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    gpio_high(GPIOB, 14);

    // // Turn on RED LED
    // gpio_init(GPIOB, 8, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
    // gpio_high(GPIOB, 8);


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

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART4EN;

    USART_REPL->BRR = 48000000 / 115200;
    USART_REPL->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}
