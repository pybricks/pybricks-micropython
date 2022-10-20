// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objmodule.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"

#include <pbio/version.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include "genhdr/mpversion.h"

STATIC const MP_DEFINE_STR_OBJ(pybricks_info_hub_obj, PYBRICKS_HUB_NAME);
STATIC const MP_DEFINE_STR_OBJ(pybricks_info_release_obj, PBIO_VERSION_STR);
STATIC const MP_DEFINE_STR_OBJ(pybricks_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);

STATIC const mp_rom_obj_tuple_t pybricks_info_obj = {
    {&mp_type_tuple},
    3,
    {
        MP_ROM_PTR(&pybricks_info_hub_obj),
        MP_ROM_PTR(&pybricks_info_release_obj),
        MP_ROM_PTR(&pybricks_info_version_obj),
    }
};

#if MICROPY_MODULE_ATTR_DELEGATION
STATIC void pb_package_pybricks_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // This will get called when external imports tries to store the module
    // as an attribute to this package. This is not currently supported, but
    // it should not cause an exception, so indicate success.
    dest[0] = MP_OBJ_NULL;
}
#endif

STATIC const mp_rom_map_elem_t pybricks_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pybricks) },
    { MP_ROM_QSTR(MP_QSTR_version),             MP_ROM_PTR(&pybricks_info_obj)},
    #if MICROPY_MODULE_ATTR_DELEGATION
    MP_MODULE_ATTR_DELEGATION_ENTRY(&pb_package_pybricks_attr),
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_package_pybricks_globals, pybricks_globals_table);

const mp_obj_module_t pb_package_pybricks = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_package_pybricks_globals,
};

#if PYBRICKS_RUNS_ON_EV3DEV
// ev3dev extends the C module in Python
MP_REGISTER_MODULE(MP_QSTR_pybricks_c, pb_package_pybricks);
#else
MP_REGISTER_MODULE(MP_QSTR_pybricks, pb_package_pybricks);
#endif

#if PYBRICKS_OPT_COMPILER
/**
 * import * from all builtin Pybricks and MicroPython modules.
 */
static void pb_package_import_all(void) {

    // Go through all modules in mp_builtin_module_map.
    for (size_t i = 0; i < mp_builtin_module_map.used; i++) {
        // This is a constant map of modules, so we can skip checks for
        // filled slots and module types.
        mp_import_all(mp_builtin_module_map.table[i].value);
    }

    #if PYBRICKS_PY_HUBS
    // Initialize hub instance
    const mp_obj_t args;
    mp_store_name(MP_QSTR_hub, pb_type_ThisHub.type.make_new(&pb_type_ThisHub.type, 0, 0, &args));
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
        pb_type_Color_reset();
        // Import all if requested.
        if (import_all) {
            pb_package_import_all();
        }
        nlr_pop();
    } else {
        // Print initialization or import exception.
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#else
// Cheaper implementation of the above. This is sufficient on builds without
// the compiler, since the following deterministic action should not raise
// exceptions as it is only called before executing anything else.
void pb_package_pybricks_init(bool import_all) {
    pb_type_Color_reset();
}
#endif // PYBRICKS_OPT_COMPILER

void pb_package_pybricks_deinit(void) {
    // Disconnect from remote.
    #if PYBRICKS_PY_PUPDEVICES
    pb_type_Remote_cleanup();
    #endif // PYBRICKS_PY_PUPDEVICES
}
