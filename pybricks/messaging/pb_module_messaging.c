// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_MESSAGING

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include <pbio/util.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/util_pb/pb_error.h>

static const mp_rom_map_elem_t messaging_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_messaging) },
};
static MP_DEFINE_CONST_DICT(pb_module_messaging_globals, messaging_globals_table);

const mp_obj_module_t pb_module_messaging = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_messaging_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_messaging, pb_module_messaging);
#endif

#endif // PYBRICKS_PY_MESSAGING
