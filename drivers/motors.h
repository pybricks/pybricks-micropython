#ifndef __NXOS_MOTORS_H__
#define __NXOS_MOTORS_H__

#include "base/types.h"

void nx_motors_init();

/*
 * motor : A == 0, B == 1, C == 2
 * brake : if == 1 brake when stoping
 */
void nx_motors_stop(U8 motor, bool brake);

/*
 * motor : A == 0, B == 1, C == 2
 * speed must be betwenn -100 and 100
 */
void nx_motors_rotate(U8 motor, S8 speed);

/*
 * motor : A == 0, B == 1, C == 2
 * angle : must be in degrees
 * speed must be betwenn -100 and 100
 * brake : if == 1 brake when stoping
 */
void nx_motors_rotate_angle(U8 motor, S8 speed, U32 angle, bool brake);

/*
 * motor : A == 0, B == 1, C == 2
 * speed must be betwenn -100 and 100
 * brake : if == 1 brake when stoping
 */
void nx_motors_rotate_time(U8 motor, S8 speed, U32 ms, bool brake);

/*
 * motor : A == 0, B == 1, C == 2
 * brake : if == 1 brake when stoping
 * note : tachymeters are set to 0 at startup
 */
U32 nx_motors_get_tach_count(U8 motor);

#endif
