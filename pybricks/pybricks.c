// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <string.h>

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objmodule.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"

#include <pbdrv/bluetooth.h>
#include <pbio/version.h>
#include <pbsys/host.h>
#include <pbsys/status.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>
#include <pybricks/tools.h>

#include "genhdr/mpversion.h"

static const MP_DEFINE_STR_OBJ(pybricks_info_hub_obj, PYBRICKS_HUB_NAME);
static const MP_DEFINE_STR_OBJ(pybricks_info_release_obj, PBIO_VERSION_STR);
static const MP_DEFINE_STR_OBJ(pybricks_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);

static const mp_rom_obj_tuple_t pybricks_info_obj = {
    {&mp_type_tuple},
    3,
    {
        MP_ROM_PTR(&pybricks_info_hub_obj),
        MP_ROM_PTR(&pybricks_info_release_obj),
        MP_ROM_PTR(&pybricks_info_version_obj),
    }
};

#if MICROPY_MODULE_ATTR_DELEGATION
void pb_package_pybricks_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // This will get called when external imports tries to store the module
    // as an attribute to this package. This is not currently supported, but
    // it should not cause an exception, so indicate success.
    dest[0] = MP_OBJ_NULL;
}
#endif

#if MICROPY_MODULE_BUILTIN_SUBPACKAGES
#if PYBRICKS_PY_EXPERIMENTAL
extern const mp_obj_module_t pb_module_experimental;
#endif
#if PYBRICKS_PY_HUBS
extern const mp_obj_module_t pb_module_hubs;
#endif
#if PYBRICKS_PY_NXTDEVICES
extern const mp_obj_module_t pb_module_nxtdevices;
#endif
#if PYBRICKS_PY_EV3DEVICES
extern const mp_obj_module_t pb_module_ev3devices;
#endif
#if PYBRICKS_PY_PUPDEVICES
extern const mp_obj_module_t pb_module_pupdevices;
#endif
#if PYBRICKS_PY_IODEVICES
extern const mp_obj_module_t pb_module_iodevices;
#endif
#if PYBRICKS_PY_MESSAGING
extern const mp_obj_module_t pb_module_messaging;
#endif
#if PYBRICKS_PY_PARAMETERS
extern const mp_obj_module_t pb_module_parameters;
#endif
#if PYBRICKS_PY_TOOLS
extern const mp_obj_module_t pb_module_tools;
#endif
#if PYBRICKS_PY_ROBOTICS
extern const mp_obj_module_t pb_module_robotics;
#endif
#endif

static const mp_rom_map_elem_t pybricks_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pybricks) },
    { MP_ROM_QSTR(MP_QSTR_version),             MP_ROM_PTR(&pybricks_info_obj)},
    #if MICROPY_MODULE_BUILTIN_SUBPACKAGES
    #if PYBRICKS_PY_EXPERIMENTAL
    { MP_ROM_QSTR(MP_QSTR_experimental), MP_ROM_PTR(&pb_module_experimental) },
    #endif
    #if PYBRICKS_PY_HUBS
    { MP_ROM_QSTR(MP_QSTR_hubs), MP_ROM_PTR(&pb_module_hubs) },
    #endif
    #if PYBRICKS_PY_NXTDEVICES
    { MP_ROM_QSTR(MP_QSTR_nxtdevices), MP_ROM_PTR(&pb_module_nxtdevices) },
    #endif
    #if PYBRICKS_PY_EV3DEVICES
    { MP_ROM_QSTR(MP_QSTR_ev3devices), MP_ROM_PTR(&pb_module_ev3devices) },
    #endif
    #if PYBRICKS_PY_PUPDEVICES
    { MP_ROM_QSTR(MP_QSTR_pupdevices), MP_ROM_PTR(&pb_module_pupdevices) },
    #endif
    #if PYBRICKS_PY_IODEVICES
    { MP_ROM_QSTR(MP_QSTR_iodevices), MP_ROM_PTR(&pb_module_iodevices) },
    #endif
    #if PYBRICKS_PY_MESSAGING
    { MP_ROM_QSTR(MP_QSTR_messaging), MP_ROM_PTR(&pb_module_messaging) },
    #endif
    #if PYBRICKS_PY_PARAMETERS
    { MP_ROM_QSTR(MP_QSTR_parameters), MP_ROM_PTR(&pb_module_parameters) },
    #endif
    #if PYBRICKS_PY_TOOLS
    { MP_ROM_QSTR(MP_QSTR_tools), MP_ROM_PTR(&pb_module_tools) },
    #endif
    #if PYBRICKS_PY_ROBOTICS
    { MP_ROM_QSTR(MP_QSTR_robotics), MP_ROM_PTR(&pb_module_robotics) },
    #endif
    #endif
};
static MP_DEFINE_CONST_DICT(pb_package_pybricks_globals, pybricks_globals_table);

const mp_obj_module_t pb_package_pybricks = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_package_pybricks_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks, pb_package_pybricks);
MP_REGISTER_MODULE_DELEGATION(pb_package_pybricks, pb_package_pybricks_attr);

#if PYBRICKS_OPT_COMPILER
/**
 * Import all MicroPython modules and import * from Pybricks modules.
 */
static void pb_package_import_all(void) {

    // Go through all modules in mp_builtin_module_map.
    for (size_t i = 0; i < mp_builtin_module_map.used; i++) {
        // This is a constant map of modules, so we can skip checks for
        // filled slots or confirming that we have module types.
        qstr module_name = MP_OBJ_QSTR_VALUE(mp_builtin_module_map.table[i].key);
        mp_obj_t module = mp_builtin_module_map.table[i].value;
        if (!strncmp("pybricks", qstr_str(module_name), 8)) {
            // Import everything from a Pybricks module.
            mp_import_all(module);
        } else {
            // Otherwise import just the module.
            mp_store_global(module_name, module);
        }
    }

    #if PYBRICKS_PY_HUBS
    // Initialize hub instance
    const mp_obj_t args;
    mp_store_name(MP_QSTR_hub, MP_OBJ_TYPE_GET_SLOT(&pb_type_ThisHub, make_new)(&pb_type_ThisHub, 0, 0, &args));
    #endif
}

/**
 * Prepares Pybricks MicroPython environment.
 *
 * @param [in]  import_all      Whether to import * from all pybricks.* modules.
 */

void pb_package_pybricks_init(bool import_all) {

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // Initialize the package.
        #if PYBRICKS_PY_PARAMETERS
        pb_type_Color_reset();
        #endif
        #if PYBRICKS_PY_TOOLS
        pb_module_tools_init();
        #endif
        // Import all if requested.
        if (import_all) {
            pb_package_import_all();
        }
        nlr_pop();
    } else {
        // Print initialization or import exception.
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
    }
}
#else
// Cheaper implementation of the above. This is sufficient on builds without
// the compiler, since the following deterministic action should not raise
// exceptions as it is only called before executing anything else.
void pb_package_pybricks_init(bool import_all) {
    pb_type_Color_reset();
    pb_module_tools_init();
}
#endif // PYBRICKS_OPT_COMPILER
