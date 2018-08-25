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

// this is a minimal IRQ and reset framework for any Cortex-M CPU

extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss;

void Reset_Handler(void) __attribute__((naked));
void Reset_Handler(void) {
    // set stack pointer
    //__asm volatile ("ldr sp, =_estack");
    // copy .data section from flash to RAM
    for (uint32_t *src = &_sidata, *dest = &_sdata; dest < &_edata;) {
        *dest++ = *src++;
    }
    // zero out .bss section
    for (uint32_t *dest = &_sbss; dest < &_ebss;) {
        *dest++ = 0;
    }
    // jump to board initialisation
    void _start(void);
    _start();
}

void Default_Handler(void) {
    for (;;) {
    }
}

const uint32_t isr_vector[] __attribute__((section(".isr_vector"))) = {
    (uint32_t)&_estack,
    (uint32_t)&Reset_Handler,
    (uint32_t)&Default_Handler, // NMI_Handler
    (uint32_t)&Default_Handler, // HardFault_Handler
    (uint32_t)&Default_Handler, // MemManage_Handler
    (uint32_t)&Default_Handler, // BusFault_Handler
    (uint32_t)&Default_Handler, // UsageFault_Handler
    0,
    0,
    0,
    0,
    (uint32_t)&Default_Handler, // SVC_Handler
    (uint32_t)&Default_Handler, // DebugMon_Handler
    0,
    (uint32_t)&Default_Handler, // PendSV_Handler
    (uint32_t)&Default_Handler, // SysTick_Handler
};

void _start(void) {
    // when we get here: stack is initialised, bss is clear, data is copied

    // SCB->CCR: enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    // *((volatile uint32_t*)0xe000ed14) |= 1 << 9;

    // initialise the cpu and peripherals
    void stm32_init(void);
    stm32_init();

    // now that we have a basic system up and running we can call main
    main(0, NULL);

    // we must not return
    for (;;) {
    }
}

// this is minimal set-up code for an STM32 MCU

// simple GPIO interface
#define GPIO_MODE_IN (0)
#define GPIO_MODE_OUT (1)
#define GPIO_MODE_ALT (2)
#define GPIO_PULL_NONE (0)
#define GPIO_PULL_UP (0)
#define GPIO_PULL_DOWN (1)
void gpio_init(GPIO_TypeDef *gpio, int pin, int mode, int pull, int alt) {
    gpio->MODER = (gpio->MODER & ~(3 << (2 * pin))) | (mode << (2 * pin));
    // OTYPER is left as default push-pull
    // OSPEEDR is left as default low speed
    gpio->PUPDR = (gpio->PUPDR & ~(3 << (2 * pin))) | (pull << (2 * pin));
    gpio->AFR[pin >> 3] = (gpio->AFR[pin >> 3] & ~(15 << (4 * (pin & 7)))) | (alt << (4 * (pin & 7)));
}
#define gpio_get(gpio, pin) ((gpio->IDR >> (pin)) & 1)
#define gpio_set(gpio, pin, value) do { gpio->ODR = (gpio->ODR & ~(1 << (pin))) | (value << pin); } while (0)
#define gpio_low(gpio, pin) do { gpio->BRR = (1 << (pin)); } while (0)
#define gpio_high(gpio, pin) do { gpio->BSRR = (1 << (pin)); } while (0)

void stm32_init(void) {
    // basic MCU config
    RCC->CR |= RCC_CR_HSION;
    RCC->CFGR = 0x00000000; // reset all
    RCC->CR &= (uint32_t)0xfef6ffff; // reset HSEON, CSSON, PLLON
    // RCC->PLLCFGR = 0x24003010; // reset PLLCFGR
    RCC->CR &= (uint32_t)0xfffbffff; // reset HSEBYP
    RCC->CIR = 0x00000000; // disable IRQs

    // leave the clock as-is (internal 16MHz)

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

    USART_REPL->BRR = 69;
    USART_REPL->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}


///////////////////////////////////
// The following is based on the tutorial at: http://micropython-dev-docs.readthedocs.io/en/latest/adding-module.html
// But I have skipped this step so far:
// The second file you will need to add to is xxxxx.ld, which is the map of
// memory used by the compiler. You have to add it to the list of files to be
// put in the .irom0.text section, so that your code goes into the instruction
// read-only memory (iROM). If you fail to do that, the compiler will try to
// put it in the instruction random-access memory (iRAM), which is a very
// scarce resource, and which can get overflown if you try to put too much
// there.


#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include <stdio.h>

STATIC mp_obj_t mymodule_gpios(mp_obj_t value) {
    mp_uint_t action = (mp_obj_get_int(value) & 0xF00) >> 8;
    mp_uint_t port = (mp_obj_get_int(value) & 0x0F0) >> 4;
    mp_uint_t pin = (mp_obj_get_int(value) & 0x00F);
    mp_uint_t retval = 2;

    GPIO_TypeDef *gpio;

    switch(port){
        case 0:
            gpio = GPIOA;
            // printf("PORT: A\n");
            break;
        case 1:
            gpio = GPIOB;
            // printf("PORT: B\n");
            break;
        case 2:
            gpio = GPIOC;
            // printf("PORT: C\n");
            break;
        case 3:
            gpio = GPIOD;
            // printf("PORT: D\n");
            break;
        case 5:
            gpio = GPIOF;
            // printf("PORT: F\n");
            break;
        default:
            printf("Unknown port\n");
            return mp_obj_new_int_from_uint(3);
    }

    switch(action){
        case 0: // Init IN UP
            gpio_init(gpio, pin, GPIO_MODE_IN, GPIO_PULL_UP, 0);
            printf("Init PIN %d as INput pull up\n", pin);
            break;
        case 1: // Init OUT
            gpio_init(gpio, pin, GPIO_MODE_OUT, GPIO_PULL_NONE, 0);
            printf("Init PIN %d as OUTput\n", pin);
            break;
        case 2: // Read
            retval = gpio_get(gpio, pin);
            // printf("Read PIN %d\n", pin);
            break;
        case 3: // SET LOW
            gpio_low(gpio, pin);
            printf("Set PIN %d low\n", pin);
            break;
        case 4: // SET HIGH
            gpio_high(gpio, pin);
            printf("Make PIN %d high\n", pin);
            break;
        case 5: // Init IN DOWN
            gpio_init(gpio, pin, GPIO_MODE_IN, GPIO_PULL_DOWN, 0);
            printf("Init PIN %d as INput pull down\n", pin);
            break;
        default:
            printf("Unknown Action \n");
            return mp_obj_new_int_from_uint(4);
    }
    return mp_obj_new_int_from_uint(retval);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mymodule_gpios_obj, mymodule_gpios);

STATIC const mp_map_elem_t mymodule_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_mymodule) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_gpios), (mp_obj_t)&mymodule_gpios_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_mymodule_globals,
    mymodule_globals_table
);

const mp_obj_module_t mp_module_mymodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_mymodule_globals,
};

///////////////////////////////////
