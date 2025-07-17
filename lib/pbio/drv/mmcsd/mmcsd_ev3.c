// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Driver for EV3 MMC/SD controller

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MMCSD_EV3

#include <pbdrv/gpio.h>
#include <pbio/os.h>

#include <tiam1808/hw/soc_AM1808.h>
#include <tiam1808/hw/hw_mmcsd.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_types.h>

#include "../drv/gpio/gpio_ev3.h"

#include "../uart/uart_debug_first_port.h"

// Init clock speed
#define MMCSD_CLK_DIV_INIT      ((SOC_SYSCLK_2_FREQ / 2 + 399999) / 400000 - 1)
// Standard SD clock speed
#define MMCSD_CLK_DIV_25M       ((SOC_SYSCLK_2_FREQ / 2 + 24999999) / 25000000 - 1)
// Not achievable, only 37.5 MHz can be reached
#define MMCSD_CLK_DIV_50M       ((SOC_SYSCLK_2_FREQ / 2 + 49999999) / 50000000 - 1)

static const pbdrv_gpio_t pin_mmcsd_clk = PBDRV_GPIO_EV3_PIN(10, 3, 0, 4, 7);
static const pbdrv_gpio_t pin_mmcsd_cmd = PBDRV_GPIO_EV3_PIN(10, 7, 4, 4, 6);
static const pbdrv_gpio_t pin_mmcsd_dat0 = PBDRV_GPIO_EV3_PIN(10, 11, 8, 4, 5);
static const pbdrv_gpio_t pin_mmcsd_dat1 = PBDRV_GPIO_EV3_PIN(10, 15, 12, 4, 4);
static const pbdrv_gpio_t pin_mmcsd_dat2 = PBDRV_GPIO_EV3_PIN(10, 19, 16, 4, 3);
static const pbdrv_gpio_t pin_mmcsd_dat3 = PBDRV_GPIO_EV3_PIN(10, 23, 20, 4, 2);
// static const pbdrv_gpio_t pin_mmcsd_card_det = PBDRV_GPIO_EV3_PIN(11, 7, 4, 5, 14);

static pbio_os_process_t ev3_mmcsd_process;

pbio_error_t ev3_mmcsd_process_thread(pbio_os_state_t *state, void *context) {
    static pbio_os_timer_t timer;
    static uint32_t rca;

    PBIO_OS_ASYNC_BEGIN(state);

    // pbdrv_uart_debug_printf("mmcsd init defer\r\n");

    // turn on clocks for 1 ms
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCLK) = (MMCSD_CLK_DIV_INIT << MMCSD_MMCCLK_CLKRT_SHIFT) | MMCSD_MMCCLK_CLKEN;
    PBIO_OS_AWAIT_MS(state, &timer, 1);

    // cmd0
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = 0;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (0 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_NORSP << MMCSD_MMCCMD_RSPFMT_SHIFT);
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
    // pbdrv_uart_debug_printf("cmd0\r\n");

    uint32_t resp[4];
    uint32_t data[512 / 4];

    // cmd8
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = 0x1aa;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (8 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R1 << MMCSD_MMCCMD_RSPFMT_SHIFT);
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
    resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
    resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
    pbdrv_uart_debug_printf("cmd8 %08x %08x\r\n", resp[0], resp[1]);

    while (1) {
        // acmd41
        HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = 0;
        HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (55 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R1 << MMCSD_MMCCMD_RSPFMT_SHIFT);
        PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
        resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
        resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
        // pbdrv_uart_debug_printf("cmd55 %08x %08x\r\n", resp[0], resp[1]);

        HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = 0x50300000;
        HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (41 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R3 << MMCSD_MMCCMD_RSPFMT_SHIFT);
        PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
        resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
        resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
        // pbdrv_uart_debug_printf("acmd41 %08x %08x\r\n", resp[0], resp[1]);

        if (resp[1] & 0x80000000) {
            pbdrv_uart_debug_printf("acmd41 %08x %08x\r\n", resp[0], resp[1]);
            break;
        }
    }

    // cmd2
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = 0;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (2 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R2 << MMCSD_MMCCMD_RSPFMT_SHIFT);
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
    resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP01);
    resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP23);
    resp[2] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
    resp[3] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
    pbdrv_uart_debug_printf("cmd2 %08x %08x %08x %08x\r\n", resp[0], resp[1], resp[2], resp[3]);

    // cmd3
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = 0;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (3 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R6 << MMCSD_MMCCMD_RSPFMT_SHIFT);
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
    resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
    resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
    pbdrv_uart_debug_printf("cmd3 %08x %08x\r\n", resp[0], resp[1]);

    rca = resp[1] & 0xffff0000;

    // Can use faster clocks now
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCLK) = (MMCSD_CLK_DIV_25M << MMCSD_MMCCLK_CLKRT_SHIFT) | MMCSD_MMCCLK_CLKEN;

    // cmd9
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = rca;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (9 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R2 << MMCSD_MMCCMD_RSPFMT_SHIFT) | MMCSD_MMCCMD_PPLEN;
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
    resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP01);
    resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP23);
    resp[2] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
    resp[3] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
    pbdrv_uart_debug_printf("cmd9 %08x %08x %08x %08x\r\n", resp[0], resp[1], resp[2], resp[3]);

    // cmd7
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = rca;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (7 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R1 << MMCSD_MMCCMD_RSPFMT_SHIFT) | MMCSD_MMCCMD_PPLEN | MMCSD_MMCCMD_BSYEXP;
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_BSYDNE);
    resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
    resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
    pbdrv_uart_debug_printf("cmd7 %08x %08x\r\n", resp[0], resp[1]);

    // acmd51

    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCFIFOCTL) = MMCSD_MMCFIFOCTL_FIFORST;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCFIFOCTL) = MMCSD_MMCFIFOCTL_ACCWD_4BYTES | (0 << MMCSD_MMCFIFOCTL_FIFODIR_SHIFT);

    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = rca;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (55 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R1 << MMCSD_MMCCMD_RSPFMT_SHIFT) | MMCSD_MMCCMD_PPLEN;
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_RSPDNE);
    resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
    resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
    // pbdrv_uart_debug_printf("cmd55 %08x %08x\r\n", resp[0], resp[1]);

    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCBLEN) = 8;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCNBLK) = 1;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCARGHL) = 0;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCMD) = (51 << MMCSD_MMCCMD_CMD_SHIFT) | (MMCSD_MMCCMD_RSPFMT_R1 << MMCSD_MMCCMD_RSPFMT_SHIFT) | MMCSD_MMCCMD_PPLEN | MMCSD_MMCCMD_WDATX;
    PBIO_OS_AWAIT_WHILE(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST1) & MMCSD_MMCST1_FIFOEMP);
    data[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCDRR);
    PBIO_OS_AWAIT_WHILE(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST1) & MMCSD_MMCST1_FIFOEMP);
    data[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCDRR);
    PBIO_OS_AWAIT_UNTIL(state, HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCST0) & MMCSD_MMCST0_DATDNE);
    resp[0] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP45);
    resp[1] = HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCRSP67);
    // pbdrv_uart_debug_printf("acmd51 %08x %08x\r\n", resp[0], resp[1]);
    pbdrv_uart_debug_printf("acmd51 %08x %08x\r\n", data[0], data[1]);

    pbdrv_uart_debug_printf("mmcsd init defer end\r\n");

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

void pbdrv_mmcsd_init(void) {
    // pbdrv_uart_debug_printf("mmcsd init\r\n");

    // reset
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCTL) = MMCSD_MMCCTL_CMDRST | MMCSD_MMCCTL_DATRST;
    HWREG(SOC_MMCSD_0_REGS + MMCSD_MMCCTL) = 0;

    // gpio
    pbdrv_gpio_alt(&pin_mmcsd_clk, SYSCFG_PINMUX10_PINMUX10_3_0_MMCSD0_CLK);
    pbdrv_gpio_alt(&pin_mmcsd_cmd, SYSCFG_PINMUX10_PINMUX10_7_4_MMCSD0_CMD);
    pbdrv_gpio_alt(&pin_mmcsd_dat0, SYSCFG_PINMUX10_PINMUX10_11_8_MMCSD0_DAT0);
    pbdrv_gpio_alt(&pin_mmcsd_dat1, SYSCFG_PINMUX10_PINMUX10_15_12_MMCSD0_DAT1);
    pbdrv_gpio_alt(&pin_mmcsd_dat2, SYSCFG_PINMUX10_PINMUX10_19_16_MMCSD0_DAT2);
    pbdrv_gpio_alt(&pin_mmcsd_dat3, SYSCFG_PINMUX10_PINMUX10_23_20_MMCSD0_DAT3);

    // defer
    pbio_os_process_start(&ev3_mmcsd_process, ev3_mmcsd_process_thread, NULL);
}

#endif // PBDRV_CONFIG_MMCSD_EV3
