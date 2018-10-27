#include "py/obj.h"
#include <mpconfigbrick.h>

const mp_obj_fun_builtin_fixed_t hub_batt_volt_obj;
const mp_obj_fun_builtin_fixed_t hub_batt_cur_obj;
const mp_obj_fun_builtin_fixed_t hub_power_off_obj;
const mp_obj_fun_builtin_fixed_t hub_reboot_obj;
const mp_obj_fun_builtin_fixed_t hub_set_light_obj;
#if PYBRICKS_ENABLE_HARDWARE_DEBUG
const mp_obj_fun_builtin_fixed_t hub_read_adc_obj;
const mp_obj_fun_builtin_fixed_t hub_gpios_obj;
#endif //PYBRICKS_ENABLE_HARDWARE_DEBUG

