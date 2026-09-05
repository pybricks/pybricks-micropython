// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/util_pb/pb_error.h>

#include "iodevices.h"

#include <pbsys/storage_settings.h>

/**
 * Interactive test for getting user permission to enable power for custom sensors.
 *
 * On first use, this also asks whether power should persist after the program
 * stops. The default is no.
 *
 * @returns  True if the user accepted now or previously, otherwise false.
 */
static bool pb_module_iodevices_has_power_permission(void) {
    // Permission persists.
    if (!pbsys_storage_settings_get_flag(PBSYS_STORAGE_SETTINGS_FLAGS_SENSOR_POWER_SAFETY_PROMPT_ACCEPTED)) {
        mp_printf(&mp_plat_print, "Enabling 8V power on self-build circuits may damage your LEGO hub. Proceed? (y/n)\n");
        int chr = mp_hal_stdin_rx_chr();
        if (chr == 'y' || chr == 'Y') {
            mp_printf(&mp_plat_print, "%c. power_pin may be enabled at your own risk.\n", chr);
            pbsys_storage_settings_set_flag(PBSYS_STORAGE_SETTINGS_FLAGS_SENSOR_POWER_SAFETY_PROMPT_ACCEPTED, true);
        } else {
            mp_printf(&mp_plat_print, "Cancelled. power_pin will be ignored.\n", chr);
            return false;
        }
    }

    // After the safety confirmation, ask once whether power should persist.
    if (!pbsys_storage_settings_get_flag(PBSYS_STORAGE_SETTINGS_FLAGS_SENSOR_POWER_PERSIST_PROMPT_SHOWN)) {
        mp_printf(&mp_plat_print, "Persist 8V power after program stops? (y/n)\n");
        int chr = mp_hal_stdin_rx_chr();
        pbsys_storage_settings_set_flag(PBSYS_STORAGE_SETTINGS_FLAGS_SENSOR_POWER_PERSIST_PROMPT_SHOWN, true);
        if (chr == 'y' || chr == 'Y') {
            mp_printf(&mp_plat_print, "%c. Power will stay on after the program stops.\n", chr);
            pbsys_storage_settings_set_flag(PBSYS_STORAGE_SETTINGS_FLAGS_SENSOR_POWER_PERSIST, true);
        } else {
            mp_printf(&mp_plat_print, "Power will turn off when the program stops.\n");
        }
    }

    return pbsys_storage_settings_get_flag(PBSYS_STORAGE_SETTINGS_FLAGS_SENSOR_POWER_SAFETY_PROMPT_ACCEPTED);
}

/**
 * Gets whether the user opted in to keeping custom port power on after stop.
 *
 * @returns  True if persistent power is enabled.
 */
bool pb_module_iodevices_get_persist_power(void) {
    return pbsys_storage_settings_get_flag(PBSYS_STORAGE_SETTINGS_FLAGS_SENSOR_POWER_PERSIST);
}

/**
 * Gets power requirement flag from user argument and asks for permission if needed.
 *
 * @param [in]  power_pin_in  User argument for power pin requirement.
 * @returns                   Power requirement flag to use for requested I/O operation if valid and permitted.
 */
pbio_port_power_requirements_t pb_module_iodevices_get_requested_power_pin(mp_obj_t power_pin_in) {

    pbio_port_power_requirements_t pin = mp_obj_get_int(power_pin_in);

    // No power always passes.
    if (pin == PBIO_PORT_POWER_REQUIREMENTS_NONE) {
        return pin;
    }

    // Allow only valid values.
    if (pin != PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P1_POS && pin != PBIO_PORT_POWER_REQUIREMENTS_BATTERY_VOLTAGE_P2_POS) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Using pins requires permission.
    if (!pb_module_iodevices_has_power_permission()) {
        pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    }

    return pin;
}

static const mp_rom_map_elem_t iodevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_iodevices)                },
    #if PYBRICKS_PY_IODEVICES_ANALOG_SENSOR
    { MP_ROM_QSTR(MP_QSTR_AnalogSensor),     MP_ROM_PTR(&pb_type_iodevices_AnalogSensor)   },
    #endif
    #if PYBRICKS_PY_IODEVICES_DC_MOTOR
    { MP_ROM_QSTR(MP_QSTR_DCMotor),          MP_ROM_PTR(&pb_type_DCMotor)                  },
    #endif
    #if PYBRICKS_PY_IODEVICES_I2C_DEVICE
    { MP_ROM_QSTR(MP_QSTR_I2CDevice),        MP_ROM_PTR(&pb_type_i2c_device)               },
    #endif
    #if PYBRICKS_PY_IODEVICES_LUMP_DEVICE
    { MP_ROM_QSTR(MP_QSTR_LUMPDevice),       MP_ROM_PTR(&pb_type_iodevices_PUPDevice)      },
    #endif
    #if PYBRICKS_PY_IODEVICES_LWP3_DEVICE
    { MP_ROM_QSTR(MP_QSTR_LWP3Device),       MP_ROM_PTR(&pb_type_lwp3device)               },
    #endif
    #if PYBRICKS_PY_IODEVICES_PUP_DEVICE
    { MP_ROM_QSTR(MP_QSTR_PUPDevice),        MP_ROM_PTR(&pb_type_iodevices_PUPDevice)      },
    #endif
    #if PYBRICKS_PY_IODEVICES_UART_DEVICE
    { MP_ROM_QSTR(MP_QSTR_UARTDevice),       MP_ROM_PTR(&pb_type_uart_device)              },
    #endif
    #if PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER
    { MP_ROM_QSTR(MP_QSTR_XboxController),   MP_ROM_PTR(&pb_type_iodevices_XboxController) },
    #endif
};
static MP_DEFINE_CONST_DICT(pb_module_iodevices_globals, iodevices_globals_table);

const mp_obj_module_t pb_module_iodevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_iodevices_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_iodevices, pb_module_iodevices);
#endif

#endif // PYBRICKS_PY_IODEVICES
