#define PYBRICKS_BRICK_MOVEHUB
#define PYBRICKS_BRICK_NAME             "BOOST Move Hub"

extern const struct _mp_obj_module_t mp_module_hub;
extern const struct _mp_obj_module_t mp_module_constants;

#define PYBRICKS_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_hub), (mp_obj_t)&mp_module_hub },  \
    { MP_OBJ_NEW_QSTR(MP_QSTR__constants), (mp_obj_t)&mp_module_constants },  
