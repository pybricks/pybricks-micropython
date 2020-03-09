// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include "py/mpconfig.h"

#include <pbdrv/adc.h>
#include <pbdrv/button.h>
#include <pbdrv/uart.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "pberror.h"


STATIC mp_obj_t debug_read_adc(mp_obj_t ch) {
    uint16_t value;

    pb_assert(pbdrv_adc_get_ch(mp_obj_get_int(ch), &value));

    return mp_obj_new_int(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(debug_read_adc_obj, debug_read_adc);

STATIC mp_obj_t debug_read_buttons() {
    pbio_button_flags_t flags;

    pb_assert(pbdrv_button_is_pressed(&flags));

    return mp_obj_new_int(flags);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(debug_read_buttons_obj, debug_read_buttons);

STATIC mp_obj_t debug_uart_baud(mp_obj_t id_obj, mp_obj_t baud_obj) {
    mp_int_t id = mp_obj_get_int(id_obj);
    mp_int_t baud = mp_obj_get_int(baud_obj);
    pbdrv_uart_dev_t *uart_dev;
    pb_assert(pbdrv_uart_get(id, &uart_dev));
    pb_assert(pbdrv_uart_set_baud_rate(uart_dev, baud));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(debug_uart_baud_obj, debug_uart_baud);

STATIC mp_obj_t debug_uart_read(mp_obj_t id_obj, mp_obj_t size_obj) {
    nlr_buf_t nlr;
    uint8_t *buf;
    pbdrv_uart_dev_t *uart_dev;
    pbio_error_t err;
    mp_int_t id = mp_obj_get_int(id_obj);
    mp_int_t size = mp_obj_get_int(size_obj);
    if (size < 1) {
        mp_raise_ValueError("size < 1");
    }
    buf = m_new(uint8_t, size);

    pb_assert(pbdrv_uart_get(id, &uart_dev));
    pb_assert(pbdrv_uart_read_begin(uart_dev, buf, size, 5000));
    if (nlr_push(&nlr) == 0) {
        while ((err = pbdrv_uart_read_end(uart_dev)) == PBIO_ERROR_AGAIN) {
            MICROPY_EVENT_POLL_HOOK
        }
        nlr_pop();
        pb_assert(err);
    } else {
        pbdrv_uart_read_cancel(uart_dev);
        while (pbdrv_uart_read_end(uart_dev) == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP
        }
        nlr_jump(nlr.ret_val);
    }
    return mp_obj_new_bytearray_by_ref(size, buf);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(debug_uart_read_obj, debug_uart_read);

STATIC mp_obj_t debug_uart_write(mp_obj_t id_obj, mp_obj_t bytes_obj) {
    nlr_buf_t nlr;
    mp_buffer_info_t bufinfo;
    pbdrv_uart_dev_t *uart_dev;
    pbio_error_t err;
    mp_int_t id = mp_obj_get_int(id_obj);
    mp_get_buffer_raise(bytes_obj, &bufinfo, MP_BUFFER_READ);

    pb_assert(pbdrv_uart_get(id, &uart_dev));
    pb_assert(pbdrv_uart_write_begin(uart_dev, bufinfo.buf, bufinfo.len, 1000));
    if (nlr_push(&nlr) == 0) {
        while ((err = pbdrv_uart_write_end(uart_dev)) == PBIO_ERROR_AGAIN) {
            MICROPY_EVENT_POLL_HOOK
        }
        nlr_pop();
        pb_assert(err);
    } else {
        pbdrv_uart_write_cancel(uart_dev);
        while (pbdrv_uart_write_end(uart_dev) == PBIO_ERROR_AGAIN) {
            MICROPY_VM_HOOK_LOOP
        }
        nlr_jump(nlr.ret_val);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(debug_uart_write_obj, debug_uart_write);

STATIC const mp_rom_map_elem_t debug_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_debug)          },
    { MP_ROM_QSTR(MP_QSTR_read_adc),        MP_ROM_PTR(&debug_read_adc_obj)     },
    { MP_ROM_QSTR(MP_QSTR_read_buttons),    MP_ROM_PTR(&debug_read_buttons_obj) },
    { MP_ROM_QSTR(MP_QSTR_uart_baud),       MP_ROM_PTR(&debug_uart_baud_obj)    },
    { MP_ROM_QSTR(MP_QSTR_uart_read),       MP_ROM_PTR(&debug_uart_read_obj)    },
    { MP_ROM_QSTR(MP_QSTR_uart_write),      MP_ROM_PTR(&debug_uart_write_obj)   },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_debug_globals, debug_globals_table);

const mp_obj_module_t pb_module_debug = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_debug_globals,
};
