# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (c) 2019-2022 The Pybricks Authors

PBIO_PLATFORM = nxt

# ensure required git submodules checked out
ifeq ("$(wildcard ../../micropython/README.md)","")
$(info GIT cloning micropython submodule)
$(info $(shell cd ../.. && git submodule update --init micropython))
ifeq ("$(wildcard ../../micropython/README.md)","")
$(error failed)
endif
endif
ifeq ("$(wildcard ../../lib/libfixmath/README.md)","")
$(info GIT cloning libfixmath submodule)
$(info $(shell cd ../.. && git submodule update --init lib/libfixmath))
ifeq ("$(wildcard ../../lib/libfixmath/README.md)","")
$(error failed)
endif
endif
ifeq ("$(wildcard nxt-firmware-drivers/README.md)","")
$(info GIT cloning nxt-firmware-drivers submodule)
$(info $(shell cd ../.. && git submodule update --checkout --init bricks/nxt/nxt-firmware-drivers))
ifeq ("$(wildcard nxt-firmware-drivers/README.md)","")
$(error failed)
endif
endif

# lets micropython make files work with external files
USER_C_MODULES = ../..

include ../../micropython/py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = ../pybricks_qstrdefs.h
QSTR_GLOBAL_DEPENDENCIES =

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

MPY_CROSS = ../../micropython/mpy-cross/mpy-cross
MPY_CROSS_FLAGS += -mno-unicode

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE ?= arm-none-eabi-

# Bricks must specify the following variables in their Makefile
ifeq ($(PBIO_PLATFORM),)
$(error "PBIO_PLATFORM is not specified - add it in <hub>/Makefile)
endif

INC += -I.
INC += -I$(TOP)
INC += -I../../lib/contiki-core
INC += -I../../lib/lego
INC += -I../../lib/libfixmath/libfixmath
INC += -Inxt-firmware-drivers
INC += -Inxt-firmware-drivers/nxt
INC += -I../../lib/pbio/include
INC += -I../../lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I../../lib/pbio
INC += -I../..
INC += -I$(BUILD)

COPT += -DFIXMATH_NO_CTYPE -DFIXMATH_NO_CACHE -DFIXMATH_FAST_SIN

CFLAGS_MCU = -mthumb -mtune=arm7tdmi -mcpu=arm7tdmi -msoft-float
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU) $(COPT) $(CFLAGS_EXTRA)
LDSCRIPT = $(PBIO_PLATFORM).ld
LDFLAGS = -nostdlib -T $(LDSCRIPT) -Map=$@.map --cref --gc-sections

# avoid doubles
CFLAGS += -fsingle-precision-constant -Wdouble-promotion

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

LIBS = "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

SRC_C = \
	main.c \
	systick.c \

# Extra core MicroPython files

# NB: Since we are using MicroPython's build system, files in the micropython/
# directory have the micropython/ prefix excluded. It is very important to do
# it that way since there is special handling of certain files that will break
# if we don't do it this way. So we need to be very careful about name clashes
# between the top level directory and the micropython/ subdirectory.

SRC_C += $(addprefix shared/,\
	libc/string0.c \
	readline/readline.c \
	runtime/interrupt_char.c \
	runtime/pyexec.c \
	runtime/stdout_helpers.c \
	)

SRC_S = \
	shared/runtime/gchelper_arm7tdmi.s \

SRC_C_NXT := $(addprefix nxt-firmware-drivers/nxt/,\
	aic.c \
	bt.c \
	data_abort.c \
	display.c \
	flashprog.c \
	flashwrite.c \
	hs.c \
	i2c.c \
	nxt_avr.c \
	nxt_lcd.c \
	nxt_motors.c \
	nxt_spi.c \
	printf.c \
	sensors.c \
	sound.c \
	systick.c \
	twi.c \
	udp.c \
	usart.c \
	)

SRC_S_NXT := $(addprefix nxt-firmware-drivers/nxt/,\
	init.s \
	interrupts.s \
	irq.s \
	vectors.s \
	)

# Pybricks modules
PYBRICKS_PYBRICKS_SRC_C = $(addprefix pybricks/,\
	pybricks.c \
	common/pb_type_battery.c \
	common/pb_type_control.c \
	common/pb_type_dcmotor.c \
	common/pb_type_logger.c \
	common/pb_type_motor.c \
	common/pb_type_system.c \
	experimental/pb_module_experimental.c \
	geometry/pb_module_geometry.c \
	geometry/pb_type_matrix.c \
	hubs/pb_module_hubs.c \
	hubs/pb_type_nxtbrick.c \
	media/pb_module_media.c \
	parameters/pb_type_icon.c \
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
	parameters/pb_type_port.c \
	parameters/pb_type_side.c \
	parameters/pb_type_stop.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_drivebase.c \
	tools/pb_module_tools.c \
	tools/pb_type_stopwatch.c \
	util_mp/pb_obj_helper.c \
	util_mp/pb_type_enum.c \
	util_pb/pb_device_stm32.c \
	util_pb/pb_error.c \
	)

CONTIKI_SRC_C = $(addprefix lib/contiki-core/,\
	sys/autostart.c \
	sys/etimer.c \
	sys/process.c \
	sys/timer.c \
	)

LIBFIXMATH_SRC_C = $(addprefix lib/libfixmath/libfixmath/,\
	fix16_sqrt.c \
	fix16_str.c \
	fix16_trig.c \
	fix16.c \
	uint32.c \
	)

PBIO_SRC_C = $(addprefix lib/pbio/,\
	drv/battery/battery_nxt.c \
	drv/button/button_nxt.c \
	drv/clock/clock_nxt.c \
	drv/core.c \
	drv/counter/counter_core.c \
	drv/counter/counter_nxt.c \
	drv/motor_driver/motor_driver_nxt.c \
	drv/reset/reset_nxt.c \
	platform/motors/settings.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	platform/$(PBIO_PLATFORM)/sys.c \
	src/battery.c \
	src/color/conversion.c \
	src/control.c \
	src/dcmotor.c \
	src/drivebase.c \
	src/error.c \
	src/integrator.c \
	src/logger.c \
	src/main.c \
	src/math.c \
	src/motor_process.c \
	src/observer.c \
	src/parent.c \
	src/servo.c \
	src/tacho.c \
	src/trajectory.c \
	sys/battery.c \
	sys/command.c \
	sys/hmi.c \
	sys/status.c \
	sys/supervisor.c \
	sys/user_program.c \
	)

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C_NXT:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S_NXT:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(CONTIKI_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIBFIXMATH_SRC_C:.c=.o))
OBJ += $(BUILD)/main.mpy.o

$(BUILD)/main.mpy: main.py
	$(ECHO) "MPY $<"
	$(Q)$(MPY_CROSS) -o $@ $(MPY_CROSS_FLAGS) $<
	$(ECHO) "`wc -c < $@` bytes"

$(BUILD)/main.mpy.o: $(BUILD)/main.mpy
	$(Q)$(OBJCOPY) -I binary -O elf32-littlearm -B arm \
		--rename-section .data=.mpy,alloc,load,readonly,data,contents $^ $@

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

all: $(BUILD)/firmware.bin

$(BUILD)/firmware.elf: $(LDSCRIPT) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

# firmware blob with main.mpy - can be flashed to hub
$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware file"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data -j .name -j .user $^ $@
	$(ECHO) "`wc -c < $@` bytes"

include $(TOP)/py/mkrules.mk
