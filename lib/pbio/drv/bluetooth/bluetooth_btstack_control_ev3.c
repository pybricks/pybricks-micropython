#include "bluetooth_btstack_control_ev3.h"

#if PBDRV_CONFIG_BLUETOOTH_BTSTACK_EV3

#include <math.h>

#include <btstack.h>
#include <tiam1808/ecap.h>
#include <tiam1808/hw/hw_syscfg0_AM1808.h>
#include <tiam1808/hw/hw_syscfg1_AM1808.h>
#include <tiam1808/hw/hw_types.h>
#include <tiam1808/hw/soc_AM1808.h>

#include <pbio/error.h>
#include <pbio/os.h>
#include "../gpio/gpio_ev3.h"

static const pbdrv_gpio_t pin_bluetooth_enable = PBDRV_GPIO_EV3_PIN(9, 27, 24, 4, 9);

static int ev3_control_off();

static void ev3_control_init(const void *config) {
    // From the ev3dev configuration:
    //
    // There is a PIC microcontroller for interfacing with an Apple MFi
    // chip. This interferes with normal Bluetooth operation, so we need
    // to make sure it is turned off. Note: The publicly available
    // schematics from LEGO don't show that these pins are connected to
    // anything, but they are present in the source code from LEGO.
    const pbdrv_gpio_t bt_pic_en = PBDRV_GPIO_EV3_PIN(8, 19, 16, 3, 3);
    pbdrv_gpio_alt(&bt_pic_en, SYSCFG_PINMUX8_PINMUX8_19_16_GPIO3_3);
    pbdrv_gpio_out_low(&bt_pic_en);
    // Hold RTS high (we're not ready to receive anything from the PIC).
    const pbdrv_gpio_t bt_pic_rts = PBDRV_GPIO_EV3_PIN(9, 7, 4, 4, 14);
    pbdrv_gpio_alt(&bt_pic_rts, SYSCFG_PINMUX9_PINMUX9_7_4_GPIO4_14);
    pbdrv_gpio_out_high(&bt_pic_rts);
    // CTS technically does not need to be configured, but for documentation
    // purposes we do.
    const pbdrv_gpio_t bt_pic_cts = PBDRV_GPIO_EV3_PIN(12, 3, 0, 5, 7);
    pbdrv_gpio_alt(&bt_pic_cts, SYSCFG_PINMUX12_PINMUX12_3_0_GPIO5_7);
    pbdrv_gpio_input(&bt_pic_cts);
    // Don't interfere with the BT clock's enable pin.
    const pbdrv_gpio_t bt_clock_en = PBDRV_GPIO_EV3_PIN(1, 11, 8, 0, 5);
    pbdrv_gpio_alt(&bt_clock_en, SYSCFG_PINMUX1_PINMUX1_11_8_GPIO0_5);
    pbdrv_gpio_input(&bt_clock_en);

    // Configure ECAP2 to emit the slow clock signal for the bluetooth module.
    ECAPOperatingModeSelect(SOC_ECAP_2_REGS, ECAP_APWM_MODE);
    // Calculate the number of clock ticks the APWM period should last. Note
    // that the following float operations are all constant and optimized away.
    // APWM is clocked by sysclk2 by default.
    // Target frequency is 32.767 kHz, see cc2560 datasheet.
    // Note that the APWM module wraps on the cycle after reaching the period
    // value, which means we need to subtract one from the desired period to get
    // a period length in cycles that matches the desired frequency.
    const int aprd = round(SOC_SYSCLK_2_FREQ / 32767.0) - 1;
    ECAPAPWMCaptureConfig(SOC_ECAP_2_REGS, aprd / 2, aprd);
    // Set the polarity to active high. It doesn't matter which it is but for
    // the sake of determinism we set it explicitly.
    ECAPAPWMPolarityConfig(SOC_ECAP_2_REGS, ECAP_APWM_ACTIVE_HIGH);
    // Disable input and output synchronization.
    ECAPSyncInOutSelect(SOC_ECAP_2_REGS, ECAP_SYNC_IN_DISABLE, ECAP_SYNC_OUT_DISABLE);
    // Start the counter running.
    ECAPCounterControl(SOC_ECAP_2_REGS, ECAP_COUNTER_FREE_RUNNING);
    // Set gp0[7] to output the ECAP2 APWM signal.
    const pbdrv_gpio_t bluetooth_slow_clock = PBDRV_GPIO_EV3_PIN(1, 3, 0, 0, 7);
    pbdrv_gpio_alt(&bluetooth_slow_clock, SYSCFG_PINMUX1_PINMUX1_3_0_ECAP2);

    pbdrv_gpio_alt(&pin_bluetooth_enable, SYSCFG_PINMUX9_PINMUX9_27_24_GPIO4_9);

    // Start the module in a defined (and disabled) state.
    ev3_control_off();
}

static int ev3_control_on() {
    // Note: the module is not actually "on" yet, however, the way it signals
    // its on-ness is by unblocking our ability to send UART messages. We
    // use auto flow control on the UART, so we don't actually need to wait
    // for the module to come up here.
    pbdrv_gpio_out_high(&pin_bluetooth_enable);
    return 0;
}

static int ev3_control_off() {
    pbdrv_gpio_out_low(&pin_bluetooth_enable);
    return 0;
}

static const btstack_control_t ev3_control = {
    .init = &ev3_control_init,
    .on = &ev3_control_on,
    .off = &ev3_control_off,
    .sleep = NULL,
    .wake = NULL,
    .register_for_power_notifications = NULL,
};

const btstack_control_t *pbdrv_bluetooth_control_ev3_instance(void) {
    return &ev3_control;
}

#endif
