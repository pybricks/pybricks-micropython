# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

###############################
# Standard MicroPython config #
###############################

# Enable/disable modules and 3rd-party libs to be included in interpreter

# Build 32-bit binaries on a 64-bit host
MICROPY_FORCE_32BIT = 0

# This variable can take the following values:
#  0 - no readline, just simple stdin input
#  1 - use MicroPython version of readline
MICROPY_USE_READLINE = 1

# btree module using Berkeley DB 1.xx
MICROPY_PY_BTREE = 1

# _thread module using pthreads
MICROPY_PY_THREAD = 1

# Subset of CPython termios module
MICROPY_PY_TERMIOS = 1

# Subset of CPython socket module
MICROPY_PY_SOCKET = 1

# ffi module requires libffi (libffi-dev Debian package)
MICROPY_PY_FFI = 1

# ussl module requires one of the TLS libraries below
MICROPY_PY_USSL = 1
# axTLS has minimal size but implements only a subset of modern TLS
# functionality, so may have problems with some servers.
MICROPY_SSL_AXTLS = 1
# mbedTLS is more up to date and complete implementation, but also
# more bloated.
MICROPY_SSL_MBEDTLS = 0

# jni module requires JVM/JNI
MICROPY_PY_JNI = 0

# Avoid using system libraries, use copies bundled with MicroPython
# as submodules (currently affects only libffi).
MICROPY_STANDALONE = 0


######################
# Pybricks additions #
######################

USER_C_MODULES = ../../..

INC += -I../../..
INC += -I../../../lib/contiki-core
INC += -I../../../lib/libfixmath/libfixmath
INC += -I../../../lib/lego
INC += -I../../../lib/pbio/include
INC += -I../../../lib/pbio/platform/virtual_hub

# Pybricks drivers and modules

EXTMOD_SRC_C += $(addprefix pybricks/,\
	common/pb_type_battery.c \
	common/pb_type_colorlight_external.c \
	common/pb_type_colorlight_internal.c \
	common/pb_type_control.c \
	common/pb_type_dcmotor.c \
	common/pb_type_keypad.c \
	common/pb_type_lightarray.c \
	common/pb_type_logger.c \
	common/pb_type_motor.c \
	common/pb_type_system.c \
	ev3devices/pb_module_ev3devices.c \
	experimental/pb_module_experimental.c \
	geometry/pb_module_geometry.c \
	geometry/pb_type_matrix.c \
	hubs/pb_module_hubs.c \
	hubs/pb_type_virtualhub.c \
	iodevices/pb_module_iodevices.c \
	iodevices/pb_type_iodevices_analogsensor.c \
	iodevices/pb_type_iodevices_i2cdevice.c \
	iodevices/pb_type_iodevices_lumpdevice.c \
	iodevices/pb_type_iodevices_lwp3device.c \
	iodevices/pb_type_iodevices_pupdevice.c \
	iodevices/pb_type_iodevices_uartdevice.c \
	media/pb_module_media.c \
	nxtdevices/pb_module_nxtdevices.c \
	nxtdevices/pb_type_nxtdevices_colorsensor.c \
	nxtdevices/pb_type_nxtdevices_energymeter.c \
	nxtdevices/pb_type_nxtdevices_lightsensor.c \
	nxtdevices/pb_type_nxtdevices_soundsensor.c \
	nxtdevices/pb_type_nxtdevices_temperaturesensor.c \
	nxtdevices/pb_type_nxtdevices_touchsensor.c \
	nxtdevices/pb_type_nxtdevices_ultrasonicsensor.c \
	parameters/pb_module_parameters.c \
	parameters/pb_type_button.c \
	parameters/pb_type_color.c \
	parameters/pb_type_direction.c \
	parameters/pb_type_icon.c \
	parameters/pb_type_port.c \
	parameters/pb_type_side.c \
	parameters/pb_type_stop.c \
	pupdevices/pb_module_pupdevices.c \
	pupdevices/pb_type_pupdevices_colordistancesensor.c \
	pupdevices/pb_type_pupdevices_colorlightmatrix.c \
	pupdevices/pb_type_pupdevices_colorsensor.c \
	pupdevices/pb_type_pupdevices_forcesensor.c \
	pupdevices/pb_type_pupdevices_infraredsensor.c \
	pupdevices/pb_type_pupdevices_light.c \
	pupdevices/pb_type_pupdevices_pfmotor.c \
	pupdevices/pb_type_pupdevices_remote.c \
	pupdevices/pb_type_pupdevices_tiltsensor.c \
	pupdevices/pb_type_pupdevices_ultrasonicsensor.c \
	pybricks.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_drivebase.c \
	tools/pb_module_tools.c \
	tools/pb_type_stopwatch.c \
	util_mp/pb_obj_helper.c \
	util_mp/pb_type_enum.c \
	util_pb/pb_color_map.c \
	util_pb/pb_conversions.c \
	util_pb/pb_device_stm32.c \
	util_pb/pb_error.c \
	util_pb/pb_flash.c \
	util_pb/pb_task.c \
	)

LIB_SRC_C += $(addprefix lib/,\
	contiki-core/sys/autostart.c \
	contiki-core/sys/etimer.c \
	contiki-core/sys/process.c \
	contiki-core/sys/timer.c \
	libfixmath/libfixmath/fix16_sqrt.c \
	libfixmath/libfixmath/fix16_str.c \
	libfixmath/libfixmath/fix16.c \
	libfixmath/libfixmath/uint32.c \
	pbio/drv/battery/battery_virtual.c \
	pbio/drv/button/button_virtual.c \
	pbio/drv/clock/clock_linux.c \
	pbio/drv/clock/clock_virtual.c \
	pbio/drv/core.c \
	pbio/drv/counter/counter_core.c \
	pbio/drv/counter/counter_virtual.c \
	pbio/drv/ioport/ioport_virtual.c \
	pbio/drv/led/led_core.c \
	pbio/drv/led/led_virtual.c \
	pbio/drv/motor_driver/motor_driver_virtual.c \
	pbio/drv/virtual.c \
	pbio/platform/virtual_hub/sys.c \
	pbio/src/battery.c \
	pbio/src/color/conversion.c \
	pbio/src/control.c \
	pbio/src/dcmotor.c \
	pbio/src/drivebase.c \
	pbio/src/error.c \
	pbio/src/integrator.c \
	pbio/src/iodev.c \
	pbio/src/light/animation.c \
	pbio/src/light/color_light.c \
	pbio/src/logger.c \
	pbio/src/main.c \
	pbio/src/math.c \
	pbio/src/motor_process.c \
	pbio/src/motor/servo_settings.c \
	pbio/src/observer.c \
	pbio/src/parent.c \
	pbio/src/servo.c \
	pbio/src/tacho.c \
	pbio/src/task.c \
	pbio/src/trajectory.c \
	pbio/src/uartdev.c \
	pbio/sys/battery.c \
	pbio/sys/bluetooth.c \
	pbio/sys/command.c \
	pbio/sys/hmi.c \
	pbio/sys/io_ports.c \
	pbio/sys/light_matrix.c \
	pbio/sys/light.c \
	pbio/sys/main.c \
	pbio/sys/status.c \
	pbio/sys/supervisor.c \
	pbio/sys/user_program.c \
	)

# realtime library for timer signals
LIB += -lrt

# embedded Python

EMBEDED_PYTHON ?= python3.8
PYTHON_CONFIG := $(EMBEDED_PYTHON)-config

INC += $(shell $(PYTHON_CONFIG) --includes)
LIB += -l$(EMBEDED_PYTHON)
LDFLAGS += $(shell $(PYTHON_CONFIG) --ldflags)
