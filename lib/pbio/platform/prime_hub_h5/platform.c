// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2026 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>

#include <btstack_chipset_cc256x.h>
#undef UNUSED
#include <stm32h5xx_hal.h>
#include <stm32h5xx_ll_dma.h>
#include <stm32h5xx_ll_rcc.h>

#include <pbdrv/clock.h>
#include <pbdrv/ioport.h>
#include "pbio/light_matrix.h"
#include <pbio/port_interface.h>

#include "../../drv/adc/adc_stm32_hal.h"
#include "../../drv/block_device/block_device_w25qxx_stm32.h"
#include "../../drv/bluetooth/bluetooth_btstack_stm32_hal.h"
#include "../../drv/bluetooth/bluetooth_btstack.h"
#include "../../drv/charger/charger_mp2639a.h"
#include "../../drv/imu/imu_lsm6ds3tr_c_stm32.h"
#include "../../drv/led/led_array_pwm.h"
#include "../../drv/led/led_dual.h"
#include "../../drv/led/led_pwm.h"
#include "../../drv/motor_driver/motor_driver_hbridge_pwm.h"
#include "../../drv/pwm/pwm_stm32_tim.h"
#include "../../drv/pwm/pwm_tlc5955_stm32.h"
#include "../../drv/resistor_ladder/resistor_ladder.h"
#include "../../drv/reset/reset_stm32.h"
#include "../../drv/sound/sound_stm32_hal_dac.h"
#include "../../drv/uart/uart_stm32_ll_irq.h"
#include "../../drv/usb/usb_stm32.h"

// Special symbols for firmware compatibility with mboot
typedef struct {
    uint8_t hub_ui_color;
    char mcu_name[15];
    uint8_t unknown;
    char board_name[15];
} lego_fw_info_t;

// Mboot reads this, so must be in a known location.
const lego_fw_info_t __attribute__((section(".fw_info"), used)) fw_info = {
    .hub_ui_color = 0x0d, /* Must be less than 0x0F. */ \
    .mcu_name = "STM32H562", \
    .unknown = 0x00, \
    .board_name = "SPIKE Prime", \
};

typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} pbdrv_prime_hub_h5_fault_stack_frame_t;

typedef enum {
    PBDRV_PRIME_HUB_H5_FAULT_HARD = 1,
    PBDRV_PRIME_HUB_H5_FAULT_MEMMANAGE = 2,
    PBDRV_PRIME_HUB_H5_FAULT_BUS = 3,
    PBDRV_PRIME_HUB_H5_FAULT_USAGE = 4,
    PBDRV_PRIME_HUB_H5_FAULT_SECURE = 5,
} pbdrv_prime_hub_h5_fault_type_t;

typedef struct {
    uint32_t magic;
    uint32_t fault_type;
    uint32_t exc_return;
    uint32_t msp;
    uint32_t psp;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t shcsr;
    uint32_t icsr;
} pbdrv_prime_hub_h5_crash_dump_t;

#define PBDRV_PRIME_HUB_H5_CRASH_DUMP_MAGIC (0x50424835)

pbdrv_prime_hub_h5_crash_dump_t pbdrv_prime_hub_h5_crash_dump
__attribute__((section(".noinit"), used));

static void pbdrv_prime_hub_h5_panic_uart_init(void) {
    // Set Port A, pin 10 (buffer pin) to output mode
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (10 * 2))) | (1U << (10 * 2)); // Output mode = 01
    GPIOA->OTYPER &= ~(1U << 10); // Push-pull
    GPIOA->PUPDR &= ~(3U << (10 * 2)); // No pull-up/pull-down
    // Set the UART buffer pin (Port A, Pin 10) low to enable output buffer (active low).
    GPIOA->BSRR = GPIO_BSRR_BR10;

    // Set Port A p5 (GPIOD pin 7) and p6 (GPIOD pin 13) to input mode
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    // Clear mode bits for pin 7 and 13 (input mode = 00)
    GPIOD->MODER &= ~((3U << (7 * 2)) | (3U << (13 * 2)));
    // Optionally, clear pull-up/pull-down for these pins
    GPIOD->PUPDR &= ~((3U << (7 * 2)) | (3U << (13 * 2)));

    // Port A UART is UART7 with TX on PE8 and RX on PE7.
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
    RCC->APB1LENR |= RCC_APB1LENR_UART7EN;

    GPIOE->MODER = (GPIOE->MODER & ~(GPIO_MODER_MODE7_Msk | GPIO_MODER_MODE8_Msk))
        | (2U << GPIO_MODER_MODE7_Pos)
        | (2U << GPIO_MODER_MODE8_Pos);
    GPIOE->OTYPER &= ~(GPIO_OTYPER_OT7 | GPIO_OTYPER_OT8);
    GPIOE->OSPEEDR = (GPIOE->OSPEEDR & ~(GPIO_OSPEEDR_OSPEED7_Msk | GPIO_OSPEEDR_OSPEED8_Msk))
        | (3U << GPIO_OSPEEDR_OSPEED7_Pos)
        | (3U << GPIO_OSPEEDR_OSPEED8_Pos);
    GPIOE->PUPDR = (GPIOE->PUPDR & ~(GPIO_PUPDR_PUPD7_Msk | GPIO_PUPDR_PUPD8_Msk))
        | (1U << GPIO_PUPDR_PUPD7_Pos)
        | (1U << GPIO_PUPDR_PUPD8_Pos);

    GPIOE->AFR[0] = (GPIOE->AFR[0] & ~(0xFU << GPIO_AFRL_AFSEL7_Pos))
        | (7U << GPIO_AFRL_AFSEL7_Pos);
    GPIOE->AFR[1] = (GPIOE->AFR[1] & ~(0xFU << GPIO_AFRH_AFSEL8_Pos))
        | (7U << GPIO_AFRH_AFSEL8_Pos);

    UART7->CR1 = 0;
    UART7->CR2 = 0;
    UART7->CR3 = 0;

    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    if (pclk1 == 0) {
        pclk1 = 16000000;
    }

    UART7->BRR = (pclk1 + 115200 / 2) / 115200;
    UART7->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void pbdrv_prime_hub_h5_panic_uart_putc(char c) {
    while (!(UART7->ISR & USART_ISR_TXE_TXFNF)) {
    }
    UART7->TDR = (uint8_t)c;
}

static void pbdrv_prime_hub_h5_panic_uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            pbdrv_prime_hub_h5_panic_uart_putc('\r');
        }
        pbdrv_prime_hub_h5_panic_uart_putc(*(s++));
    }
}

// Temporary debug trace output on Port A UART (same transport as panic dump).
void pbdrv_prime_hub_h5_trace_uart_puts(const char *s) {
    static bool initialized;

    if (!initialized) {
        pbdrv_prime_hub_h5_panic_uart_init();
        initialized = true;
    }

    pbdrv_prime_hub_h5_panic_uart_puts(s);
}

static void pbdrv_prime_hub_h5_panic_uart_put_hex32(uint32_t value) {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        pbdrv_prime_hub_h5_panic_uart_putc(hex[(value >> (i * 4)) & 0xF]);
    }
}

static void pbdrv_prime_hub_h5_panic_uart_print_reg(const char *name, uint32_t value) {
    pbdrv_prime_hub_h5_panic_uart_puts(name);
    pbdrv_prime_hub_h5_panic_uart_puts(": 0x");
    pbdrv_prime_hub_h5_panic_uart_put_hex32(value);
    pbdrv_prime_hub_h5_panic_uart_puts("\n");
}

static const char *pbdrv_prime_hub_h5_fault_name(uint32_t fault_type) {
    switch (fault_type) {
        case PBDRV_PRIME_HUB_H5_FAULT_HARD:
            return "HardFault";
        case PBDRV_PRIME_HUB_H5_FAULT_MEMMANAGE:
            return "MemManage";
        case PBDRV_PRIME_HUB_H5_FAULT_BUS:
            return "BusFault";
        case PBDRV_PRIME_HUB_H5_FAULT_USAGE:
            return "UsageFault";
        case PBDRV_PRIME_HUB_H5_FAULT_SECURE:
            return "SecureFault";
        default:
            return "Unknown";
    }
}

__attribute__((used, externally_visible, noinline))
void pbdrv_prime_hub_h5_fault_handler_c(uint32_t fault_type, uint32_t exc_return,
    const pbdrv_prime_hub_h5_fault_stack_frame_t *stacked, const uint32_t *callee_regs) {
    pbdrv_prime_hub_h5_crash_dump.magic = PBDRV_PRIME_HUB_H5_CRASH_DUMP_MAGIC;
    pbdrv_prime_hub_h5_crash_dump.fault_type = fault_type;
    pbdrv_prime_hub_h5_crash_dump.exc_return = exc_return;
    pbdrv_prime_hub_h5_crash_dump.msp = __get_MSP();
    pbdrv_prime_hub_h5_crash_dump.psp = __get_PSP();

    pbdrv_prime_hub_h5_crash_dump.r0 = stacked->r0;
    pbdrv_prime_hub_h5_crash_dump.r1 = stacked->r1;
    pbdrv_prime_hub_h5_crash_dump.r2 = stacked->r2;
    pbdrv_prime_hub_h5_crash_dump.r3 = stacked->r3;
    pbdrv_prime_hub_h5_crash_dump.r4 = callee_regs[0];
    pbdrv_prime_hub_h5_crash_dump.r5 = callee_regs[1];
    pbdrv_prime_hub_h5_crash_dump.r6 = callee_regs[2];
    pbdrv_prime_hub_h5_crash_dump.r7 = callee_regs[3];
    pbdrv_prime_hub_h5_crash_dump.r8 = callee_regs[4];
    pbdrv_prime_hub_h5_crash_dump.r9 = callee_regs[5];
    pbdrv_prime_hub_h5_crash_dump.r10 = callee_regs[6];
    pbdrv_prime_hub_h5_crash_dump.r11 = callee_regs[7];
    pbdrv_prime_hub_h5_crash_dump.r12 = stacked->r12;
    pbdrv_prime_hub_h5_crash_dump.lr = stacked->lr;
    pbdrv_prime_hub_h5_crash_dump.pc = stacked->pc;
    pbdrv_prime_hub_h5_crash_dump.xpsr = stacked->xpsr;

    pbdrv_prime_hub_h5_crash_dump.cfsr = SCB->CFSR;
    pbdrv_prime_hub_h5_crash_dump.hfsr = SCB->HFSR;
    pbdrv_prime_hub_h5_crash_dump.mmfar = SCB->MMFAR;
    pbdrv_prime_hub_h5_crash_dump.bfar = SCB->BFAR;
    pbdrv_prime_hub_h5_crash_dump.shcsr = SCB->SHCSR;
    pbdrv_prime_hub_h5_crash_dump.icsr = SCB->ICSR;

    pbdrv_prime_hub_h5_panic_uart_init();
    pbdrv_prime_hub_h5_panic_uart_puts("\n*** Pybricks crash dump (Port A UART) ***\n");
    pbdrv_prime_hub_h5_panic_uart_puts("Fault: ");
    pbdrv_prime_hub_h5_panic_uart_puts(pbdrv_prime_hub_h5_fault_name(pbdrv_prime_hub_h5_crash_dump.fault_type));
    pbdrv_prime_hub_h5_panic_uart_puts("\n");

    pbdrv_prime_hub_h5_panic_uart_print_reg("EXC_RETURN", pbdrv_prime_hub_h5_crash_dump.exc_return);
    pbdrv_prime_hub_h5_panic_uart_print_reg("MSP", pbdrv_prime_hub_h5_crash_dump.msp);
    pbdrv_prime_hub_h5_panic_uart_print_reg("PSP", pbdrv_prime_hub_h5_crash_dump.psp);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R0", pbdrv_prime_hub_h5_crash_dump.r0);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R1", pbdrv_prime_hub_h5_crash_dump.r1);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R2", pbdrv_prime_hub_h5_crash_dump.r2);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R3", pbdrv_prime_hub_h5_crash_dump.r3);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R4", pbdrv_prime_hub_h5_crash_dump.r4);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R5", pbdrv_prime_hub_h5_crash_dump.r5);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R6", pbdrv_prime_hub_h5_crash_dump.r6);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R7", pbdrv_prime_hub_h5_crash_dump.r7);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R8", pbdrv_prime_hub_h5_crash_dump.r8);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R9", pbdrv_prime_hub_h5_crash_dump.r9);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R10", pbdrv_prime_hub_h5_crash_dump.r10);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R11", pbdrv_prime_hub_h5_crash_dump.r11);
    pbdrv_prime_hub_h5_panic_uart_print_reg("R12", pbdrv_prime_hub_h5_crash_dump.r12);
    pbdrv_prime_hub_h5_panic_uart_print_reg("LR", pbdrv_prime_hub_h5_crash_dump.lr);
    pbdrv_prime_hub_h5_panic_uart_print_reg("PC", pbdrv_prime_hub_h5_crash_dump.pc);
    pbdrv_prime_hub_h5_panic_uart_print_reg("XPSR", pbdrv_prime_hub_h5_crash_dump.xpsr);
    pbdrv_prime_hub_h5_panic_uart_print_reg("HFSR", pbdrv_prime_hub_h5_crash_dump.hfsr);
    pbdrv_prime_hub_h5_panic_uart_print_reg("CFSR", pbdrv_prime_hub_h5_crash_dump.cfsr);
    pbdrv_prime_hub_h5_panic_uart_print_reg("MMFAR", pbdrv_prime_hub_h5_crash_dump.mmfar);
    pbdrv_prime_hub_h5_panic_uart_print_reg("BFAR", pbdrv_prime_hub_h5_crash_dump.bfar);
    pbdrv_prime_hub_h5_panic_uart_print_reg("SHCSR", pbdrv_prime_hub_h5_crash_dump.shcsr);
    pbdrv_prime_hub_h5_panic_uart_print_reg("ICSR", pbdrv_prime_hub_h5_crash_dump.icsr);

    while (!(UART7->ISR & USART_ISR_TC)) {
    }

    __DSB();
    __ISB();
    NVIC_SystemReset();

    for (;;) {
    }
}

#define PBDRV_PRIME_HUB_H5_DEFINE_FAULT_HANDLER(_name, _type) \
    __attribute__((naked)) void _name(void) { \
        __asm volatile ( \
    "mov r1, lr                \n" \
    "tst r1, #4                \n" \
    "ite eq                    \n" \
    "mrseq r2, msp             \n" \
    "mrsne r2, psp             \n" \
    "push {r4-r11}             \n" \
    "mov r3, sp                \n" \
    "movs r0, %[fault_type]    \n" \
    "bl pbdrv_prime_hub_h5_fault_handler_c \n" \
    "b .                       \n" \
    : \
    : [fault_type] "I" (_type) \
    : "r0", "r1", "r2", "r3", "memory"); \
    }

PBDRV_PRIME_HUB_H5_DEFINE_FAULT_HANDLER(HardFault_Handler, PBDRV_PRIME_HUB_H5_FAULT_HARD)
PBDRV_PRIME_HUB_H5_DEFINE_FAULT_HANDLER(MemManage_Handler, PBDRV_PRIME_HUB_H5_FAULT_MEMMANAGE)
PBDRV_PRIME_HUB_H5_DEFINE_FAULT_HANDLER(BusFault_Handler, PBDRV_PRIME_HUB_H5_FAULT_BUS)
PBDRV_PRIME_HUB_H5_DEFINE_FAULT_HANDLER(UsageFault_Handler, PBDRV_PRIME_HUB_H5_FAULT_USAGE)
PBDRV_PRIME_HUB_H5_DEFINE_FAULT_HANDLER(SecureFault_Handler, PBDRV_PRIME_HUB_H5_FAULT_SECURE)

enum {
    LED_DEV_0_STATUS,
    LED_DEV_1_BATTERY,
    LED_DEV_2_BLUETOOTH,
    LED_DEV_3_STATUS_TOP,
    LED_DEV_4_STATUS_BOTTOM,
};

enum {
    LED_ARRAY_DEV_0_LIGHT_MATRIX,
};

enum {
    PWM_DEV_0_TIM1,
    PWM_DEV_1_TIM3,
    PWM_DEV_2_TIM4,
    PWM_DEV_3_TIM17,
    PWM_DEV_4_TIM8,
    PWM_DEV_5_TLC5955,
    PWM_DEV_6_TIM5,
};

enum {
    RESISTOR_LADDER_DEV_0,
    RESISTOR_LADDER_DEV_1,
};

enum {
    UART_PORT_A,
    UART_PORT_B,
    UART_PORT_C,
    UART_PORT_D,
    UART_PORT_E,
    UART_PORT_F,
};

// Bluetooth

const pbdrv_bluetooth_btstack_stm32_platform_data_t pbdrv_bluetooth_btstack_stm32_platform_data = {
    .enable_gpio = {
        .bank = GPIOA,
        .pin = 2,
    },
    .uart = USART2,
    .uart_irq = USART2_IRQn,
    .tx_dma = GPDMA1_Channel6,
    .tx_dma_req = LL_GPDMA1_REQUEST_USART2_TX,
    .tx_dma_irq = GPDMA1_Channel6_IRQn,
    .rx_dma = GPDMA1_Channel7,
    .rx_dma_req = LL_GPDMA1_REQUEST_USART2_RX,
    .rx_dma_irq = GPDMA1_Channel7_IRQn,
};

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        GPIO_InitTypeDef gpio_init = { };

        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF7_USART2;

        // CTS/RTS
        gpio_init.Pin = GPIO_PIN_3 | GPIO_PIN_4;
        gpio_init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOD, &gpio_init);

        // TX/RX
        gpio_init.Pin = GPIO_PIN_5 | GPIO_PIN_6;
        gpio_init.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOD, &gpio_init);
    }
}

void GPDMA1_Channel6_IRQHandler(void) {
    pbdrv_bluetooth_btstack_stm32_hal_handle_tx_dma_irq();
}

void GPDMA1_Channel7_IRQHandler(void) {
    pbdrv_bluetooth_btstack_stm32_hal_handle_rx_dma_irq();
}

void USART2_IRQHandler(void) {
    pbdrv_bluetooth_btstack_stm32_hal_handle_uart_irq();
}

const pbdrv_bluetooth_btstack_platform_data_t pbdrv_bluetooth_btstack_platform_data = {
    .transport_instance = pbdrv_bluetooth_btstack_stm32_hal_transport_instance,
    .transport_config = pbdrv_bluetooth_btstack_stm32_hal_transport_config,
    .chipset_instance = btstack_chipset_cc256x_instance,
    .control_instance = pbdrv_bluetooth_btstack_stm32_hal_control_instance,
    .er_key = (const uint8_t *)UID_BASE,
    .ir_key = (const uint8_t *)UID_BASE,
};

// charger

const pbdrv_charger_mp2639a_platform_data_t pbdrv_charger_mp2639a_platform_data = {
    .mode_gpio = {
        .bank = GPIOE,
        .pin = 0,
    },
    .chg_resistor_ladder_id = RESISTOR_LADDER_DEV_0,
    .chg_resistor_ladder_ch = PBDRV_RESISTOR_LADDER_CH_2,
    .ib_adc_ch = 3,
    .iset_pwm_id = PWM_DEV_6_TIM5,
    .iset_pwm_ch = 1,
};

// IMU

const pbdrv_imu_lsm6s3tr_c_stm32_platform_data_t pbdrv_imu_lsm6s3tr_c_stm32_platform_data = {
    .i2c = I2C2,
};

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef gpio_init = { };

    // IMU
    if (hi2c->Instance == I2C2) {
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

        // SCL
        gpio_init.Pin = GPIO_PIN_10;

        // do a quick bus reset in case IMU chip is in bad state
        gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        for (int i = 0; i < 10; i++) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);
            pbdrv_clock_busy_delay_us(1);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
            pbdrv_clock_busy_delay_us(1);
        }

        // then configure for normal use
        gpio_init.Mode = GPIO_MODE_AF_OD;
        gpio_init.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        // SDA
        gpio_init.Pin = GPIO_PIN_3;
        gpio_init.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        HAL_NVIC_SetPriority(I2C2_ER_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
        HAL_NVIC_SetPriority(I2C2_EV_IRQn, 3, 2);
        HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);

        // INT1
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        gpio_init.Pin = GPIO_PIN_4;
        gpio_init.Mode = GPIO_MODE_IT_RISING;
        gpio_init.Alternate = 0;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        HAL_NVIC_SetPriority(EXTI4_IRQn, 3, 3);
        HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    }
}

void I2C2_ER_IRQHandler(void) {
    #if PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32
    pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_er_irq();
    #endif
}

void I2C2_EV_IRQHandler(void) {
    #if PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32
    pbdrv_imu_lsm6ds3tr_c_stm32_handle_i2c_ev_irq();
    #endif
}

void EXTI4_IRQHandler(void) {
    #if PBDRV_CONFIG_IMU_LSM6S3TR_C_STM32
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
    #endif
}

// I/O ports

const pbdrv_gpio_t pbdrv_ioport_platform_data_vcc_pin = {
    .bank = GPIOE,
    .pin = 10
};

const pbdrv_ioport_platform_data_t pbdrv_ioport_platform_data[PBDRV_CONFIG_IOPORT_NUM_DEV] = {
    {
        .port_id = PBIO_PORT_ID_A,
        .motor_driver_index = 0,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = UART_PORT_A,
        .external_port_index = 0,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOD, .pin = 7  },
            .p6 = { .bank = GPIOD, .pin = 13  },
            .uart_buf = { .bank = GPIOA, .pin = 10 },
            .uart_tx = { .bank = GPIOE, .pin = 8  },
            .uart_rx = { .bank = GPIOE, .pin = 7  },
            .uart_tx_alt_uart = GPIO_AF7_UART7,
            .uart_rx_alt_uart = GPIO_AF7_UART7,
        },
        #if PBDRV_CONFIG_UART_DEBUG_FIRST_PORT
        .supported_modes = PBIO_PORT_MODE_UART,
        #else // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT
        .supported_modes = PBIO_PORT_MODE_LEGO_DCM | PBIO_PORT_MODE_UART,
        #endif
    },
    {
        .port_id = PBIO_PORT_ID_B,
        .motor_driver_index = 1,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = UART_PORT_B,
        .external_port_index = 1,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOD, .pin = 12 },
            .p6 = { .bank = GPIOD, .pin = 10 },
            .uart_buf = { .bank = GPIOA, .pin = 8 },
            .uart_tx = { .bank = GPIOD, .pin = 1 },
            .uart_rx = { .bank = GPIOD, .pin = 0 },
            .uart_tx_alt_uart = GPIO_AF8_UART4,
            .uart_rx_alt_uart = GPIO_AF8_UART4,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_DCM | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_C,
        .motor_driver_index = 2,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = UART_PORT_C,
        .external_port_index = 2,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOD, .pin = 11 },
            .p6 = { .bank = GPIOE, .pin = 4 },
            .uart_buf = { .bank = GPIOE, .pin = 5 },
            .uart_tx = { .bank = GPIOD, .pin = 8 },
            .uart_rx = { .bank = GPIOD, .pin = 9 },
            .uart_tx_alt_uart = GPIO_AF7_USART3,
            .uart_rx_alt_uart = GPIO_AF7_USART3,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_DCM | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_D,
        .motor_driver_index = 3,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = UART_PORT_D,
        .external_port_index = 3,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOC, .pin = 15 },
            .p6 = { .bank = GPIOC, .pin = 14 },
            .uart_buf = { .bank = GPIOB, .pin = 2  },
            .uart_tx = { .bank = GPIOC, .pin = 12 },
            .uart_rx = { .bank = GPIOD, .pin = 2  },
            .uart_tx_alt_uart = GPIO_AF8_UART5,
            .uart_rx_alt_uart = GPIO_AF8_UART5,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_DCM | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_E,
        .motor_driver_index = 4,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = UART_PORT_E,
        .external_port_index = 4,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOC, .pin = 13 },
            .p6 = { .bank = GPIOE, .pin = 12 },
            .uart_buf = { .bank = GPIOB, .pin = 5 },
            .uart_tx = { .bank = GPIOE, .pin = 3 },
            .uart_rx = { .bank = GPIOE, .pin = 2 },
            .uart_tx_alt_uart = GPIO_AF7_USART10,
            .uart_rx_alt_uart = GPIO_AF7_USART10,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_DCM | PBIO_PORT_MODE_UART,
    },
    {
        .port_id = PBIO_PORT_ID_F,
        .motor_driver_index = 5,
        .i2c_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .uart_driver_index = UART_PORT_F,
        .external_port_index = 5,
        .counter_driver_index = PBDRV_IOPORT_INDEX_NOT_AVAILABLE,
        .pins = &(pbdrv_ioport_pins_t) {
            .p5 = { .bank = GPIOB, .pin = 12 },
            .p6 = { .bank = GPIOE, .pin = 6 },
            .uart_buf = { .bank = GPIOC, .pin = 5 },
            .uart_tx = { .bank = GPIOD, .pin = 15 },
            .uart_rx = { .bank = GPIOD, .pin = 14 },
            .uart_tx_alt_uart = GPIO_AF11_UART9,
            .uart_rx_alt_uart = GPIO_AF11_UART9,
        },
        .supported_modes = PBIO_PORT_MODE_LEGO_DCM | PBIO_PORT_MODE_UART,
    },
};

// LED

const pbdrv_led_dual_platform_data_t pbdrv_led_dual_platform_data[PBDRV_CONFIG_LED_DUAL_NUM_DEV] = {
    {
        .id = LED_DEV_0_STATUS,
        .id1 = LED_DEV_3_STATUS_TOP,
        .id2 = LED_DEV_4_STATUS_BOTTOM,
    },
};

static const pbdrv_led_pwm_platform_color_t pbdrv_led_pwm_color = {
    .r_factor = 1000,
    .g_factor = 170,
    .b_factor = 200,
    .r_brightness = 174,
    .g_brightness = 1590,
    .b_brightness = 327,
};

const pbdrv_led_pwm_platform_data_t pbdrv_led_pwm_platform_data[PBDRV_CONFIG_LED_PWM_NUM_DEV] = {
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_3_STATUS_TOP,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 5,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 4,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 3,
        .scale_factor = 35,
    },
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_4_STATUS_BOTTOM,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 8,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 7,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 6,
        .scale_factor = 35,
    },
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_1_BATTERY,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 2,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 1,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 0,
        .scale_factor = 35,
    },
    {
        .color = &pbdrv_led_pwm_color,
        .id = LED_DEV_2_BLUETOOTH,
        .r_id = PWM_DEV_5_TLC5955,
        .r_ch = 20,
        .g_id = PWM_DEV_5_TLC5955,
        .g_ch = 19,
        .b_id = PWM_DEV_5_TLC5955,
        .b_ch = 18,
        .scale_factor = 35,
    },
};

const pbdrv_led_array_pwm_platform_data_t pbdrv_led_array_pwm_platform_data[PBDRV_CONFIG_LED_ARRAY_PWM_NUM_DEV] = {
    {
        .pwm_chs = (const uint8_t[]) {
            38, 36, 41, 46, 33,
            37, 28, 39, 47, 21,
            24, 29, 31, 45, 23,
            26, 27, 32, 34, 22,
            25, 40, 30, 35, 9
        },
        .num_pwm_chs = 25,
        .pwm_id = PWM_DEV_5_TLC5955,
        .id = LED_ARRAY_DEV_0_LIGHT_MATRIX,
    },
};

// Motor driver

const pbdrv_motor_driver_hbridge_pwm_platform_data_t
    pbdrv_motor_driver_hbridge_pwm_platform_data[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    // Port A
    {
        .pin1_gpio.bank = GPIOE,
        .pin1_gpio.pin = 9,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = PWM_DEV_0_TIM1,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOE,
        .pin2_gpio.pin = 11,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = PWM_DEV_0_TIM1,
        .pin2_pwm_ch = 2,
    },
    // Port B
    {
        .pin1_gpio.bank = GPIOE,
        .pin1_gpio.pin = 13,
        .pin1_alt = GPIO_AF1_TIM1,
        .pin1_pwm_id = PWM_DEV_0_TIM1,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOE,
        .pin2_gpio.pin = 14,
        .pin2_alt = GPIO_AF1_TIM1,
        .pin2_pwm_id = PWM_DEV_0_TIM1,
        .pin2_pwm_ch = 4,
    },
    // Port C
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 6,
        .pin1_alt = GPIO_AF2_TIM4,
        .pin1_pwm_id = PWM_DEV_2_TIM4,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 7,
        .pin2_alt = GPIO_AF2_TIM4,
        .pin2_pwm_id = PWM_DEV_2_TIM4,
        .pin2_pwm_ch = 2,
    },
    // Port D
    {
        .pin1_gpio.bank = GPIOB,
        .pin1_gpio.pin = 8,
        .pin1_alt = GPIO_AF2_TIM4,
        .pin1_pwm_id = PWM_DEV_2_TIM4,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 9,
        .pin2_alt = GPIO_AF2_TIM4,
        .pin2_pwm_id = PWM_DEV_2_TIM4,
        .pin2_pwm_ch = 4,
    },
    // Port E
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 6,
        .pin1_alt = GPIO_AF2_TIM3,
        .pin1_pwm_id = PWM_DEV_1_TIM3,
        .pin1_pwm_ch = 1,
        .pin2_gpio.bank = GPIOC,
        .pin2_gpio.pin = 7,
        .pin2_alt = GPIO_AF2_TIM3,
        .pin2_pwm_id = PWM_DEV_1_TIM3,
        .pin2_pwm_ch = 2,
    },
    // Port F
    {
        .pin1_gpio.bank = GPIOC,
        .pin1_gpio.pin = 8,
        .pin1_alt = GPIO_AF2_TIM3,
        .pin1_pwm_id = PWM_DEV_1_TIM3,
        .pin1_pwm_ch = 3,
        .pin2_gpio.bank = GPIOB,
        .pin2_gpio.pin = 1,
        .pin2_alt = GPIO_AF2_TIM3,
        .pin2_pwm_id = PWM_DEV_1_TIM3,
        .pin2_pwm_ch = 4,
    },
};

// PWM

static void pwm_dev_0_platform_init(void) {
}

static void pwm_dev_1_platform_init(void) {
}

static void pwm_dev_2_platform_init(void) {
}

static void pwm_dev_3_platform_init(void) {
    GPIO_InitTypeDef gpio_init = { };

    gpio_init.Pin = GPIO_PIN_2;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF1_TIM17;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    // channel 1 has constant 50% duty cycle since it acts as a clock
    TIM17->CCR1 = TIM17->ARR / 2;
}

static void pwm_dev_4_platform_init(void) {
    GPIO_InitTypeDef gpio_init = { };

    gpio_init.Pin = GPIO_PIN_9;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    // channel 4 has constant 50% duty cycle since it acts as a clock
    TIM8->CCR4 = TIM8->ARR / 2;
}

static void pwm_dev_6_platform_init(void) {
    GPIO_InitTypeDef gpio_init = { };

    gpio_init.Pin = GPIO_PIN_0;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

// NOTE: Official LEGO firmware uses 1.2 kHz PWM for motors. We have changed to
// 12 kHz to reduce the unpleasant noise (similar to the frequency used by the
// official EV3 firmware).

const pbdrv_pwm_stm32_tim_platform_data_t
    pbdrv_pwm_stm32_tim_platform_data[PBDRV_CONFIG_PWM_STM32_TIM_NUM_DEV] = {
    {
        .platform_init = pwm_dev_0_platform_init,
        .TIMx = TIM1,
        .prescalar = 21, // results in ~11.9 MHz clock
        .period = 1000, // ~11.9MHz divided by 1k makes ~11.9 kHz PWM
        .id = PWM_DEV_0_TIM1,
        // channel 1/2: Port A motor driver; channel 3/4: Port B motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_1_platform_init,
        .TIMx = TIM3,
        .prescalar = 21, // results in ~11.9 MHz clock
        .period = 1000, // ~11.9MHz divided by 1k makes ~11.9 kHz PWM
        .id = PWM_DEV_1_TIM3,
        // channel 1/2: Port E motor driver; channel 3/4: Port F motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_2_platform_init,
        .TIMx = TIM4,
        .prescalar = 21, // results in ~11.9 MHz clock
        .period = 1000, // ~11.9MHz divided by 1k makes ~11.9 kHz PWM
        .id = PWM_DEV_2_TIM4,
        // channel 1/2: Port C motor driver; channel 3/4: Port D motor driver
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_2_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_ENABLE | PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE
            | PBDRV_PWM_STM32_TIM_CHANNEL_1_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_2_INVERT
            | PBDRV_PWM_STM32_TIM_CHANNEL_3_INVERT | PBDRV_PWM_STM32_TIM_CHANNEL_4_INVERT,
    },
    {
        .platform_init = pwm_dev_3_platform_init,
        .TIMx = TIM17,
        .prescalar = 2, // results in 125 MHz clock
        .period = 13, // 125MHz divided by 13 makes ~9.6 MHz PWM
        .id = PWM_DEV_3_TIM17,
        // channel 1: TLC5955 GSCLK signal
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE,
    },
    {
        .platform_init = pwm_dev_4_platform_init,
        .TIMx = TIM8,
        .prescalar = 8, // results in 31.25 MHz clock
        .period = 953, // 31.25MHz divided by 953 makes ~32.8 kHz PWM
        .id = PWM_DEV_4_TIM8,
        // channel 4: Bluetooth 32.768 kHz clock
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_4_ENABLE,
    },
    {
        .platform_init = pwm_dev_6_platform_init,
        .TIMx = TIM5,
        .prescalar = 26, // results in ~9.6 MHz clock
        .period = 100, // ~9.6MHz divided by 100 makes ~96 kHz PWM
        .id = PWM_DEV_6_TIM5,
        // channel 1: Battery charger ISET
        .channels = PBDRV_PWM_STM32_TIM_CHANNEL_1_ENABLE,
    },
};

const pbdrv_pwm_tlc5955_stm32_platform_data_t
    pbdrv_pwm_tlc5955_stm32_platform_data[PBDRV_CONFIG_PWM_TLC5955_STM32_NUM_DEV] = {
    {
        .spi = SPI1,
        .spi_irq = SPI1_IRQn,
        .rx_dma = GPDMA2_Channel2,
        .rx_dma_req = LL_GPDMA2_REQUEST_SPI1_RX,
        .rx_dma_irq = GPDMA2_Channel2_IRQn,
        .tx_dma = GPDMA2_Channel3,
        .tx_dma_req = LL_GPDMA2_REQUEST_SPI1_TX,
        .tx_dma_irq = GPDMA2_Channel3_IRQn,
        .lat_gpio = GPIOA,
        .lat_gpio_pin = GPIO_PIN_15,
        .id = PWM_DEV_5_TLC5955,
    },
};

// Reset

void pbdrv_reset_power_off(void) {
    // setting PE15 low cuts the power
    GPIOE->BSRR = GPIO_BSRR_BR15;
}

// resistor ladder

// note: even though the resistors of each ladder have the same values, there
// must be some variations in the loads, so different levels are required
const pbdrv_resistor_ladder_platform_data_t pbdrv_resistor_ladder_platform_data[] = {
    [RESISTOR_LADDER_DEV_0] = {
        .level = { 3642, 3142, 2879, 2634, 2449, 2209, 2072, 1800 },
        .adc_ch = 4,
    },
    [RESISTOR_LADDER_DEV_1] = {
        .level = { 3872, 3394, 3009, 2755, 2538, 2327, 2141, 1969 },
        .adc_ch = 5,
    },
};

// Sound
#if PBDRV_CONFIG_SOUND
const pbdrv_sound_stm32_hal_dac_platform_data_t pbdrv_sound_stm32_hal_dac_platform_data = {
    .enable_gpio_bank = GPIOB,
    .enable_gpio_pin = GPIO_PIN_14,
    .dac = DAC1,
    .dac_ch = DAC_CHANNEL_1,
    .dac_trigger = DAC_TRIGGER_T6_TRGO,
    .dma = GPDMA1_Channel5,
    .dma_req = LL_GPDMA1_REQUEST_DAC1_CH1,
    .dma_irq = GPDMA1_Channel5_IRQn,
    .tim = TIM6,
    .tim_clock_rate = 250000000, // APB1: 250MHz
};

void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac) {
    GPIO_InitTypeDef gpio_init = { };

    gpio_init.Pin = GPIO_PIN_4;
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);
}

void GPDMA1_Channel5_IRQHandler(void) {
    pbdrv_sound_stm32_hal_dac_handle_dma_irq();
}
#endif

// UART

const pbdrv_uart_stm32_ll_irq_platform_data_t
    pbdrv_uart_stm32_ll_irq_platform_data[PBDRV_CONFIG_UART_STM32_LL_IRQ_NUM_UART] = {
    [UART_PORT_A] = {
        .uart = UART7,
        .irq = UART7_IRQn,
    },
    [UART_PORT_B] = {
        .uart = UART4,
        .irq = UART4_IRQn,
    },
    [UART_PORT_C] = {
        .uart = USART3,
        .irq = USART3_IRQn,
    },
    [UART_PORT_D] = {
        .uart = UART5,
        .irq = UART5_IRQn,
    },
    [UART_PORT_E] = {
        .uart = USART10,
        .irq = USART10_IRQn,
    },
    [UART_PORT_F] = {
        .uart = UART9,
        .irq = UART9_IRQn,
    },
};

// overrides weak function in startup.s
void UART4_IRQHandler(void) {
    pbdrv_uart_stm32_ll_irq_handle_irq(UART_PORT_B);
}

// overrides weak function in startup.s
void UART5_IRQHandler(void) {
    pbdrv_uart_stm32_ll_irq_handle_irq(UART_PORT_D);
}

// overrides weak function in startup.s
void UART7_IRQHandler(void) {
    pbdrv_uart_stm32_ll_irq_handle_irq(UART_PORT_A);
}

// overrides weak function in startup.s
void USART3_IRQHandler(void) {
    pbdrv_uart_stm32_ll_irq_handle_irq(UART_PORT_C);
}

// overrides weak function in startup.s
void UART9_IRQHandler(void) {
    pbdrv_uart_stm32_ll_irq_handle_irq(UART_PORT_F);
}

// overrides weak function in startup.s
void USART10_IRQHandler(void) {
    pbdrv_uart_stm32_ll_irq_handle_irq(UART_PORT_E);
}

// STM32 HAL integration

// bootloader gives us 16MHz clock
uint32_t SystemCoreClock = 16000000;

// copied from system_stm32.c in stm32 port
const uint8_t AHBPrescTable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9 };
const uint8_t APBPrescTable[8] = { 0, 0, 0, 0, 1, 2, 3, 4 };

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef gpio_init = { };
    ADC_ChannelConfTypeDef adc_ch_config = { };

    // clocks are enabled in SystemInit
    assert_param(__HAL_RCC_TIM2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_DMA2_IS_CLK_ENABLED());
    assert_param(__HAL_RCC_ADC1_IS_CLK_ENABLED());

    // Reset ADC block to clear any stale state left by bootloader.
    __HAL_RCC_ADC_FORCE_RESET();
    __HAL_RCC_ADC_RELEASE_RESET();

    // All ADC channels in this sequence use the same conversion settings.
    adc_ch_config.SamplingTime = ADC_SAMPLETIME_12CYCLES_5;
    adc_ch_config.SingleDiff = ADC_SINGLE_ENDED;
    adc_ch_config.OffsetNumber = ADC_OFFSET_NONE;
    adc_ch_config.Offset = 0;
    adc_ch_config.OffsetSign = ADC_OFFSET_SIGN_NEGATIVE;
    adc_ch_config.OffsetSaturation = DISABLE;

    // All ADC pins are configured as analog inputs with no pull.
    gpio_init.Mode = GPIO_MODE_ANALOG;
    gpio_init.Pull = GPIO_NOPULL;

    // PC0, ADC_IBAT

    gpio_init.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_10;
    adc_ch_config.Rank = ADC_REGULAR_RANK_1;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC1, ADCVBAT

    gpio_init.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_11;
    adc_ch_config.Rank = ADC_REGULAR_RANK_2;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PB0, BAT_NTC

    gpio_init.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_9;
    adc_ch_config.Rank = ADC_REGULAR_RANK_3;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PA3, IBUSBCH

    gpio_init.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_15;
    adc_ch_config.Rank = ADC_REGULAR_RANK_4;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PC4, ADC_PC4

    gpio_init.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOC, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_4;
    adc_ch_config.Rank = ADC_REGULAR_RANK_5;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);

    // PA1, BUTTON2/3/4

    gpio_init.Pin = GPIO_PIN_1;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    adc_ch_config.Channel = ADC_CHANNEL_1;
    adc_ch_config.Rank = ADC_REGULAR_RANK_6;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);
}

void GPDMA2_Channel0_IRQHandler(void) {
    pbdrv_adc_stm32_hal_handle_irq();
}

void GPDMA2_Channel2_IRQHandler(void) {
    pbdrv_pwm_tlc5955_stm32_rx_dma_irq(0);
}

void GPDMA2_Channel3_IRQHandler(void) {
    pbdrv_pwm_tlc5955_stm32_tx_dma_irq(0);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        // TLC5955 LED driver
        GPIO_InitTypeDef gpio_init = { };

        gpio_init.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOA, &gpio_init);
    }
}

void HAL_XSPI_MspInit(XSPI_HandleTypeDef *hxspi) {
    if (hxspi->Instance == OCTOSPI1) {
        // External flash
        GPIO_InitTypeDef gpio_init = { };

        // OCTOSPI1_NCS: PC11 (AF11)
        gpio_init.Pin = GPIO_PIN_11;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF9_OCTOSPI1;
        HAL_GPIO_Init(GPIOC, &gpio_init);

        // OCTOSPI1_CLK: PB15 (AF10)
        gpio_init.Pin = GPIO_PIN_15;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Alternate = GPIO_AF10_OCTOSPI1;
        HAL_GPIO_Init(GPIOB, &gpio_init);

        // OCTOSPI1_IO0 (MOSI): PC3 (AF10)
        gpio_init.Pin = GPIO_PIN_3;
        gpio_init.Alternate = GPIO_AF9_OCTOSPI1;
        HAL_GPIO_Init(GPIOC, &gpio_init);

        // OCTOSPI1_IO1 (MISO): PC10 (AF11)
        gpio_init.Pin = GPIO_PIN_10;
        gpio_init.Alternate = GPIO_AF9_OCTOSPI1;
        HAL_GPIO_Init(GPIOC, &gpio_init);
    }
}

void SPI1_IRQHandler(void) {
    pbdrv_pwm_tlc5955_stm32_spi_irq(0);
}

void OCTOSPI1_IRQHandler(void) {
    pbdrv_block_device_w25qxx_stm32_spi_irq();
}

void GPDMA1_Channel4_IRQHandler(void) {
    pbdrv_block_device_w25qxx_stm32_spi_handle_tx_dma_irq();
}

void GPDMA1_Channel3_IRQHandler(void) {
    pbdrv_block_device_w25qxx_stm32_spi_handle_rx_dma_irq();
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        pbdrv_pwm_tlc5955_stm32_spi_tx_complete();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
}

void HAL_XSPI_TxCpltCallback(XSPI_HandleTypeDef *hxspi) {
    pbdrv_block_device_w25qxx_stm32_spi_tx_complete();
}

void HAL_XSPI_RxCpltCallback(XSPI_HandleTypeDef *hxspi) {
    pbdrv_block_device_w25qxx_stm32_spi_rx_complete();
}

void HAL_XSPI_CmdCpltCallback(XSPI_HandleTypeDef *hxspi) {
    pbdrv_block_device_w25qxx_stm32_spi_cmd_complete();
}

void HAL_XSPI_ErrorCallback(XSPI_HandleTypeDef *hxspi) {
    pbdrv_block_device_w25qxx_stm32_spi_error();
}

const pbdrv_block_device_w25qxx_stm32_platform_data_t pbdrv_block_device_w25qxx_stm32_platform_data = {
    .octospi = OCTOSPI1,
    .clock_prescaler = 4, // 250 MHz / (4 + 1) = 50 MHz
    .tx_dma = GPDMA1_Channel4,
    .tx_dma_req = LL_GPDMA1_REQUEST_OCTOSPI1,
    .tx_dma_irq = GPDMA1_Channel4_IRQn,
    .rx_dma = GPDMA1_Channel3,
    .rx_dma_req = LL_GPDMA1_REQUEST_OCTOSPI1,
    .rx_dma_irq = GPDMA1_Channel3_IRQn,
    .irq = OCTOSPI1_IRQn,
};

// USB

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd) {
    GPIO_InitTypeDef gpio_init = { };

    HAL_PWREx_EnableVddUSB();

    // Data pins
    gpio_init.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF10_USB;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    // VBUS pin
    gpio_init.Pin = GPIO_PIN_9;
    gpio_init.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_init);

    HAL_NVIC_SetPriority(USB_DRD_FS_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USB_DRD_FS_IRQn);
    HAL_NVIC_SetPriority(EXTI9_IRQn, 6, 1);
    HAL_NVIC_EnableIRQ(EXTI9_IRQn);

    // ensure correct initial state
    pbdrv_usb_stm32_handle_vbus_irq(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9) == GPIO_PIN_SET);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd) {
    HAL_NVIC_DisableIRQ(USB_DRD_FS_IRQn);

    // The VBUS IRQ remains enabled so that it can still
    // be triggered if the device is shut down but left
    // connected to charge. When the charging cable is
    // disconnected, the IRQ will trigger and lead to the
    // device fully powering down.
}

void USB_DRD_FS_IRQHandler(void) {
    pbdrv_usb_stm32_handle_otg_fs_irq();
}

void EXTI9_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_4) {
        pbdrv_imu_lsm6ds3tr_c_stm32_handle_int1_irq();
    } else if (pin == GPIO_PIN_9) {
        pbdrv_usb_stm32_handle_vbus_irq(true);
    }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_9) {
        pbdrv_usb_stm32_handle_vbus_irq(false);
    }
}

// Jump to mboot the same way MicroPython does: r0 holds the magic key, r1 holds
// the bootloader vector table from which the stack pointer and reset vector are
// loaded. r0 is preserved through the jump.
static __attribute__((naked, noreturn)) void enter_mboot(uint32_t r0, uint32_t bl_addr) {
    __asm volatile (
        "ldr r2, [r1, #0]\n" // get stack pointer
        "msr msp, r2\n"      // set stack pointer
        "ldr r2, [r1, #4]\n" // get reset vector
        "bx r2\n"            // branch to bootloader
        );
}

// Early initialization

// Copied from micropython/lib/stm32lib/CMSIS/STM32H5xx/Source/Templates/system_stm32h5xx.c
static void stm32lib_SystemInit(void) {
    uint32_t reg_opsr;

    /* FPU settings ------------------------------------------------------------*/
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 20U) | (3UL << 22U)); /* set CP10 and CP11 Full Access */
    #endif

    /* Reset the RCC clock configuration to the default reset state ------------*/
    /* Set HSION bit */
    RCC->CR = RCC_CR_HSION;

    /* Reset CFGR register */
    RCC->CFGR1 = 0U;
    RCC->CFGR2 = 0U;

    /* Reset HSEON, HSECSSON, HSEBYP, HSEEXT, HSIDIV, HSIKERON, CSION, CSIKERON, HSI48 and PLLxON bits */
    #if defined(RCC_CR_PLL3ON)
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSECSSON | RCC_CR_HSEBYP | RCC_CR_HSEEXT | RCC_CR_HSIDIV | RCC_CR_HSIKERON | \
        RCC_CR_CSION | RCC_CR_CSIKERON | RCC_CR_HSI48ON | RCC_CR_PLL1ON | RCC_CR_PLL2ON | RCC_CR_PLL3ON);
    #else
    RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_HSECSSON | RCC_CR_HSEBYP | RCC_CR_HSEEXT | RCC_CR_HSIDIV | RCC_CR_HSIKERON | \
        RCC_CR_CSION | RCC_CR_CSIKERON | RCC_CR_HSI48ON | RCC_CR_PLL1ON | RCC_CR_PLL2ON);
    #endif

    /* Reset PLLxCFGR register */
    RCC->PLL1CFGR = 0U;
    RCC->PLL2CFGR = 0U;
    #if defined(RCC_CR_PLL3ON)
    RCC->PLL3CFGR = 0U;
    #endif /* RCC_CR_PLL3ON */

    /* Reset PLL1DIVR register */
    RCC->PLL1DIVR = 0x01010280U;
    /* Reset PLL1FRACR register */
    RCC->PLL1FRACR = 0x00000000U;
    /* Reset PLL2DIVR register */
    RCC->PLL2DIVR = 0x01010280U;
    /* Reset PLL2FRACR register */
    RCC->PLL2FRACR = 0x00000000U;
    #if defined(RCC_CR_PLL3ON)
    /* Reset PLL3DIVR register */
    RCC->PLL3DIVR = 0x01010280U;
    /* Reset PLL3FRACR register */
    RCC->PLL3FRACR = 0x00000000U;
    #endif /* RCC_CR_PLL3ON */

    /* Reset HSEBYP bit */
    RCC->CR &= ~(RCC_CR_HSEBYP);

    /* Disable all interrupts */
    RCC->CIER = 0U;

    // Pybricks: this is done later to keep it similar to other platforms.
    /* Configure the Vector Table location add offset address ------------------*/
    // #ifdef VECT_TAB_SRAM
    // SCB->VTOR = SRAM1_BASE | VECT_TAB_OFFSET;     /* Vector Table Relocation in Internal SRAM */
    // #else
    // SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;     /* Vector Table Relocation in Internal FLASH */
    // #endif   /* VECT_TAB_SRAM */

    /* Check OPSR register to verify if there is an ongoing swap or option bytes update interrupted by a reset */
    reg_opsr = FLASH->OPSR & FLASH_OPSR_CODE_OP;
    if ((reg_opsr == FLASH_OPSR_CODE_OP) || (reg_opsr == (FLASH_OPSR_CODE_OP_2 | FLASH_OPSR_CODE_OP_1))) {
        /* Check FLASH Option Control Register access */
        if ((FLASH->OPTCR & FLASH_OPTCR_OPTLOCK) != 0U) {
            /* Authorizes the Option Byte registers programming */
            FLASH->OPTKEYR = 0x08192A3BU;
            FLASH->OPTKEYR = 0x4C5D6E7FU;
        }
        /* Launch the option bytes change operation */
        FLASH->OPTCR |= FLASH_OPTCR_OPTSTART;

        /* Lock the FLASH Option Control Register access */
        FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;
    }
}

// Copied from micropython/lib/stm32lib/CMSIS/STM32H5xx/Source/Templates/system_stm32h5xx.c
/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is CSI, SystemCoreClock will contain the CSI_VALUE(*)
  *
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**)
  *
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
  *
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(***)
  *             or HSI_VALUE(**) or CSI_VALUE(*) multiplied/divided by the PLL factors.
  *
  *         (*) CSI_VALUE is a constant defined in stm32h5xx_hal.h file (default value
  *             4 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *
  *         (**) HSI_VALUE is a constant defined in stm32h5xx_hal.h file (default value
  *              64 MHz) but the real value may vary depending on the variations
  *              in voltage and temperature.
  *
  *         (***) HSE_VALUE is a constant defined in stm32h5xx_hal.h file (default value
  *              25 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void) {
    uint32_t pllp, pllsource, pllm, pllfracen, hsivalue, tmp;
    float_t fracn1, pllvco;

    /* Get SYSCLK source -------------------------------------------------------*/
    switch (RCC->CFGR1 & RCC_CFGR1_SWS)
    {
        case 0x00UL: /* HSI used as system clock source */
            SystemCoreClock = (uint32_t)(HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> 3));
            break;

        case 0x08UL: /* CSI used as system clock  source */
            SystemCoreClock = CSI_VALUE;
            break;

        case 0x10UL: /* HSE used as system clock  source */
            SystemCoreClock = HSE_VALUE;
            break;

        case 0x18UL: /* PLL1 used as system clock source */
            /* PLL_VCO = (HSE_VALUE or HSI_VALUE or CSI_VALUE/ PLLM) * PLLN
            SYSCLK = PLL_VCO / PLLR
            */
            pllsource = (RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1SRC);
            pllm = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1M) >> RCC_PLL1CFGR_PLL1M_Pos);
            pllfracen = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1FRACEN) >> RCC_PLL1CFGR_PLL1FRACEN_Pos);
            fracn1 = (float_t)(uint32_t)(pllfracen * ((RCC->PLL1FRACR & RCC_PLL1FRACR_PLL1FRACN) >> RCC_PLL1FRACR_PLL1FRACN_Pos));

            switch (pllsource)
            {
                case 0x01UL: /* HSI used as PLL clock source */
                    hsivalue = (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> 3));
                    pllvco = ((float_t)hsivalue / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + \
                        (fracn1 / (float_t)0x2000) + (float_t)1);
                    break;

                case 0x02UL: /* CSI used as PLL clock source */
                    pllvco = ((float_t)CSI_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + \
                        (fracn1 / (float_t)0x2000) + (float_t)1);
                    break;

                case 0x03UL: /* HSE used as PLL clock source */
                    pllvco = ((float_t)HSE_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + \
                        (fracn1 / (float_t)0x2000) + (float_t)1);
                    break;

                default: /* No clock sent to PLL*/
                    pllvco = (float_t)0U;
                    break;
            }

            pllp = (((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1P) >> RCC_PLL1DIVR_PLL1P_Pos) + 1U);
            SystemCoreClock = (uint32_t)(float_t)(pllvco / (float_t)pllp);

            break;

        default:
            SystemCoreClock = HSI_VALUE;
            break;
    }
    /* Compute HCLK clock frequency --------------------------------------------*/
    /* Get HCLK prescaler */
    tmp = AHBPrescTable[((RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos)];
    /* HCLK clock frequency */
    SystemCoreClock >>= tmp;
}

#define ST_DEVICE_SIGNATURE_BASE (0x08fff800)
#define ST_DEVICE_SIGNATURE_LIMIT (0x08ffffff)

// copied from micropython/ports/stm32/mpu.h
static inline void mpu_init(void) {
    // Configure attribute 0, inner-outer non-cacheable (=0x44).
    __DMB();
    MPU->MAIR0 = (MPU->MAIR0 & ~MPU_MAIR0_Attr0_Msk)
        | 0x44 << MPU_MAIR0_Attr0_Pos;

    // Configure region 0 to make device signature non-cacheable.
    // This allows the memory region at ST_DEVICE_SIGNATURE_BASE to be readable.
    __DMB();
    MPU->RNR = MPU_REGION_NUMBER0;
    MPU->RBAR = (ST_DEVICE_SIGNATURE_BASE & MPU_RBAR_BASE_Msk)
        | MPU_ACCESS_NOT_SHAREABLE << MPU_RBAR_SH_Pos
        | MPU_REGION_ALL_RW << MPU_RBAR_AP_Pos
        | MPU_INSTRUCTION_ACCESS_DISABLE << MPU_RBAR_XN_Pos;
    MPU->RLAR = (ST_DEVICE_SIGNATURE_LIMIT & MPU_RLAR_LIMIT_Msk)
        | MPU_ATTRIBUTES_NUMBER0 << MPU_RLAR_AttrIndx_Pos
        | MPU_REGION_ENABLE << MPU_RLAR_EN_Pos;

    // Enable the MPU.
    MPU->CTRL = MPU_PRIVILEGED_DEFAULT | MPU_CTRL_ENABLE_Msk;
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
    __DMB();
    __ISB();
}

#define IRQ_PRI_SYSTICK NVIC_EncodePriority(NVIC_PRIORITYGROUP_4, 0, 0)

// copied from micropython/ports/stm32/powerctrlboot.c
// Pybricks changes: don't use MICROPY_* config - hard-code options instead
// Please don't remove dead code or MICROPY_* comments to keep this easy to
// compare with the original source code.
static void SystemClock_Config(void) {
    // Set power voltage scaling.
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    #if 0 // MICROPY_HW_CLK_USE_HSI
    LL_RCC_HSI_Enable();
    while (!LL_RCC_HSI_IsReady()) {
    }
    const uint32_t pll1_source = LL_RCC_PLL1SOURCE_HSI;
    #else
    // Enable HSE.
    #if 0 // MICROPY_HW_CLK_USE_BYPASS
    LL_RCC_HSE_EnableBypass();
    #endif
    LL_RCC_HSE_Enable();
    while (!LL_RCC_HSE_IsReady()) {
    }
    const uint32_t pll1_source = LL_RCC_PLL1SOURCE_HSE;
    #endif

    // Configure PLL1 for use as system clock.
    LL_RCC_PLL1_ConfigDomain_SYS(pll1_source,
        4 /* MICROPY_HW_CLK_PLLM */,
        125 /* MICROPY_HW_CLK_PLLN */,
        2 /* MICROPY_HW_CLK_PLLP */);
    LL_RCC_PLL1_SetFRACN(0 /* MICROPY_HW_CLK_PLLFRAC */);
    LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_4_8 /* MICROPY_HW_CLK_PLLVCI_LL */);
    LL_RCC_PLL1_SetVCOOutputRange(LL_RCC_PLLVCORANGE_WIDE /* MICROPY_HW_CLK_PLLVCO_LL */);
    LL_RCC_PLL1P_Enable();

    #if 1 // defined(MICROPY_HW_CLK_PLLQ)
    LL_RCC_PLL1_SetQ(5 /* MICROPY_HW_CLK_PLLQ */);
    LL_RCC_PLL1Q_Enable();
    #endif

    #if 1 // defined(MICROPY_HW_CLK_PLLR)
    LL_RCC_PLL1_SetR(2 /* MICROPY_HW_CLK_PLLR */);
    LL_RCC_PLL1R_Enable();
    #endif

    // Enable PLL1.
    LL_RCC_PLL1_Enable();
    while (!LL_RCC_PLL1_IsReady()) {
    }

    // Configure bus dividers.
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);

    // Configure the flash latency before switching the system clock source.
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_5 /* MICROPY_HW_FLASH_LATENCY */);
    while (__HAL_FLASH_GET_LATENCY() != FLASH_LATENCY_5 /* MICROPY_HW_FLASH_LATENCY */) {
    }

    // Switch the system clock source to PLL1P.
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
    }

    // Reconfigure clock state and SysTick.
    SystemCoreClockUpdate();

    // Pybricks change: inline powerctrl_config_systick();
    {
        uint32_t systick_source_freq = HAL_RCC_GetHCLKFreq();

        // Configure SYSTICK to run at 1kHz (1ms interval)
        SysTick->CTRL |= SYSTICK_CLKSOURCE_HCLK;
        SysTick_Config(systick_source_freq / 1000);
        NVIC_SetPriority(SysTick_IRQn, IRQ_PRI_SYSTICK);

        // Set SysTick IRQ priority variable in case the HAL needs to use it
        uwTickPrio = IRQ_PRI_SYSTICK;
    }

    // USB clock configuration, either HSI48 or PLL3.
    #if 1 // MICROPY_HW_ENABLE_USB && !MICROPY_HW_CLK_USE_PLL3_FOR_USB

    // Enable HSI48.
    LL_RCC_HSI48_Enable();
    while (!LL_RCC_HSI48_IsReady()) {
    }

    // Select HSI48 for USB clock source
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);

    // Synchronise HSI48 with 1kHz USB SoF
    __HAL_RCC_CRS_CLK_ENABLE();
    CRS->CFGR = 2 << CRS_CFGR_SYNCSRC_Pos | 0x22 << CRS_CFGR_FELIM_Pos
        | __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000, 1000) << CRS_CFGR_RELOAD_Pos;
    CRS->CR = 0x20 << CRS_CR_TRIM_Pos | CRS_CR_AUTOTRIMEN | CRS_CR_CEN;

    #elif 0 // MICROPY_HW_ENABLE_USB && MICROPY_HW_CLK_USE_PLL3_FOR_USB

    // Configure PLL3 for use by USB at Q=48MHz.
    LL_RCC_PLL3_SetSource(LL_RCC_PLL3SOURCE_HSE);
    LL_RCC_PLL3_SetM(16 /* MICROPY_HW_CLK_PLL3M */);
    LL_RCC_PLL3_SetN(192 /* MICROPY_HW_CLK_PLL3N */);
    LL_RCC_PLL3_SetP(2 /* MICROPY_HW_CLK_PLL3P */);
    LL_RCC_PLL3_SetQ(4 /* MICROPY_HW_CLK_PLL3Q */);
    LL_RCC_PLL3_SetR(2 /* MICROPY_HW_CLK_PLL3R */);
    LL_RCC_PLL3_SetFRACN(0 /* MICROPY_HW_CLK_PLL3FRAC */);
    LL_RCC_PLL3_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_1_2 /* MICROPY_HW_CLK_PLL3VCI_LL */);
    LL_RCC_PLL3_SetVCOOutputRange(LL_RCC_PLLVCORANGE_MEDIUM /* MICROPY_HW_CLK_PLL3VCO_LL */);
    LL_RCC_PLL3Q_Enable();

    // Enable PLL3.
    LL_RCC_PLL3_Enable();
    while (!LL_RCC_PLL3_IsReady()) {
    }

    // Select PLL3-Q for USB clock source
    LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_PLL3Q);

    #endif

    #ifdef NDEBUG
    DBGMCU->CR = 0;
    #endif
}

// special memory addresses defined in linker script
extern uint32_t *_fw_isr_vector_src;

// Called from assembly code in startup.s
void SystemInit(void) {
    // If update mode was requested before a reset, the watchdog is now cleared.
    // Enter mboot here, before anything (including the watchdog) is started.
    if (pbdrv_reset_stm32_is_bootloader_requested()) {
        extern uint32_t _pbdrv_reset_mboot_start[];
        enter_mboot(0x70ad0000, (uint32_t)_pbdrv_reset_mboot_start);
    }

    // enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    // SCB->CCR |= SCB_CCR_STKALIGN_Msk;
    // Implementation is based on stm32_main() from MicroPython.

    stm32lib_SystemInit();

    // Pybricks: using same linker script variable name as other Pybricks platforms
    SCB->VTOR = (uint32_t)&_fw_isr_vector_src;

    HAL_ICACHE_Enable();

    mpu_init();

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    // SysTick is needed by HAL_RCC_ClockConfig (called in SystemClock_Config)
    HAL_InitTick(TICK_INT_PRIORITY);

    // set the system clock to be HSE
    SystemClock_Config();

    // enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN | RCC_AHB1ENR_GPDMA2EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN |
        RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_GPIOEEN | RCC_AHB2ENR_ADCEN | RCC_AHB2ENR_DAC1EN;
    RCC->AHB4ENR |= RCC_AHB4ENR_OCTOSPI1EN;
    RCC->APB1LENR |= RCC_APB1LENR_USART2EN | RCC_APB1LENR_USART3EN | RCC_APB1LENR_UART4EN |
        RCC_APB1LENR_UART5EN | RCC_APB1LENR_UART7EN | RCC_APB1LENR_TIM2EN | RCC_APB1LENR_TIM3EN |
        RCC_APB1LENR_TIM4EN | RCC_APB1LENR_TIM5EN | RCC_APB1LENR_TIM6EN | RCC_APB1LENR_I2C2EN |
        RCC_APB1LENR_USART10EN;
    RCC->APB1HENR |= RCC_APB1HENR_UART9EN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN | RCC_APB2ENR_TIM8EN | RCC_APB2ENR_TIM17EN |
        RCC_APB2ENR_SPI1EN | RCC_APB2ENR_USBEN;
    RCC->APB3ENR |= RCC_APB3ENR_SBSEN;

    // Keep main power on (PE15)
    GPIO_InitTypeDef gpio_init = {
        .Pin = GPIO_PIN_15,
        .Mode = GPIO_MODE_OUTPUT_PP,
    };
    HAL_GPIO_Init(GPIOE, &gpio_init);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);
}
