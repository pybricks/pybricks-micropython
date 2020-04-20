// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <glib.h>
#include <gio/gio.h>

#include "py/mpconfig.h"

#include "py/obj.h"
#include "py/runtime.h"


// Gets the Bluetooth address for a paired device. name_in can be device name
// or Bluetooth address. This is intended to be somewhat equivelent to
// socket.gethostbyname() for IPv4 addresses.
STATIC mp_obj_t ev3dev_bluetooth_resolve(mp_obj_t name_in) {
    const char *name_str = mp_obj_str_get_str(name_in);
    GError *error = NULL;
    MP_THREAD_GIL_EXIT();
    GDBusObjectManager *object_manager = g_dbus_object_manager_client_new_for_bus_sync(
        G_BUS_TYPE_SYSTEM, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
        "org.bluez", "/", NULL, NULL, NULL, NULL, &error);
    MP_THREAD_GIL_ENTER();
    if (!object_manager) {
        mp_obj_t ex = mp_obj_new_exception_msg_varg(&mp_type_RuntimeError, "%s", error->message);
        g_error_free(error);
        nlr_raise(ex);
    }

    mp_obj_t match = mp_const_none;
    GList *objects = g_dbus_object_manager_get_objects(object_manager);
    for (GList *o = objects; o != NULL; o = o->next) {
        GDBusObject *obj = G_DBUS_OBJECT(o->data);
        GDBusProxy *device = G_DBUS_PROXY(g_dbus_object_get_interface(obj, "org.bluez.Device1"));
        if (!device) {
            continue;
        }
        GVariant *paired_v = g_dbus_proxy_get_cached_property(device, "Paired");
        gboolean paired = g_variant_get_boolean(paired_v);
        g_variant_unref(paired_v);
        if (!paired) {
            g_object_unref(device);
            continue;
        }

        GVariant *address_v = g_dbus_proxy_get_cached_property(device, "Address");
        gsize address_len;
        const gchar *address_str = g_variant_get_string(address_v, &address_len);
        GVariant *alias_v = g_dbus_proxy_get_cached_property(device, "Alias");
        const gchar *alias_str = g_variant_get_string(alias_v, NULL);
        if (g_ascii_strcasecmp(address_str, name_str) == 0 || g_strcmp0(alias_str, name_str) == 0) {
            match = mp_obj_new_str(address_str, address_len);
        }

        g_variant_unref(alias_v);
        g_variant_unref(address_v);
        g_object_unref(device);

        if (match != mp_const_none) {
            break;
        }
    }
    g_list_free_full(objects, g_object_unref);
    g_object_unref(object_manager);

    return match;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3dev_bluetooth_resolve_obj, ev3dev_bluetooth_resolve);

STATIC const mp_rom_map_elem_t ev3dev_bluetooth_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_bluetooth_c) },
    { MP_ROM_QSTR(MP_QSTR_resolve), MP_ROM_PTR(&ev3dev_bluetooth_resolve_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3dev_bluetooth_globals, ev3dev_bluetooth_globals_table);

const mp_obj_module_t pb_module_bluetooth = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&ev3dev_bluetooth_globals,
};
