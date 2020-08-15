# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (C) 2019-2020 The Pybricks Authors

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
QSTR_GLOBAL_DEPENDENCIES =

FROZEN_MPY_TOOL_ARGS = -mlongint-impl=none

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
INC += -I../../lib/pbio/include
INC += -I../../lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I../../lib/pbio
INC += -I../..
INC += -I$(BUILD)

COPT += -DFIXMATH_NO_CTYPE

CFLAGS_MCU = -mthumb -mtune=arm7tdmi -mcpu=arm7tdmi -msoft-float
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU) $(COPT)
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

LIBS = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

SRC_C = \
	main.c \
	systick.c \

SRC_C += $(addprefix micropython/lib/,\
	libc/string0.c \
	mp-readline/readline.c \
	utils/interrupt_char.c \
	utils/pyexec.c \
	utils/stdout_helpers.c \
	)

SRC_S = \
	micropython/lib/utils/gchelper_arm7tdmi.s \

SRC_C_NXT := $(addprefix nxt-firmware-drivers/nxt/,\
	aic.c \
	bt.c \
	data_abort.c \
	display.c \
	flashprog.c \
	flashwrite.c \
	hs.c \
	i2c.c \
	maininit.c \
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
	experimental/pb_module_experimental.c \
	hubs/pb_module_hubs.c \
	hubs/pb_type_nxtbrick.c \
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
	parameters/pb_type_stop.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_drivebase.c \
	robotics/pb_type_matrix.c \
	tools/pb_module_tools.c \
	tools/pb_type_stopwatch.c \
	util_mp/pb_obj_helper.c \
	util_mp/pb_type_enum.c \
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
	fix16.c \
	uint32.c \
	)

PBIO_SRC_C = $(addprefix lib/pbio/,\
	drv/$(PBIO_PLATFORM)/motor.c \
	drv/battery/battery_nxt.c \
	drv/button/button_nxt.c \
	drv/core.c \
	drv/counter/counter_core.c \
	drv/counter/counter_nxt.c \
	drv/reset/reset_nxt.c \
	platform/$(PBIO_PLATFORM)/clock.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	platform/$(PBIO_PLATFORM)/sys.c \
	src/color/conversion.c \
	src/control.c \
	src/dcmotor.c \
	src/drivebase.c \
	src/error.c \
	src/integrator.c \
	src/logger.c \
	src/main.c \
	src/math.c \
	src/motorpoll.c \
	src/servo.c \
	src/tacho.c \
	src/trajectory_ext.c \
	src/trajectory.c \
	sys/battery.c \
	sys/status.c \
	sys/supervisor.c \
	)

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C_NXT:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S_NXT:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(CONTIKI_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIBFIXMATH_SRC_C:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

all: $(BUILD)/firmware.bin

$(BUILD)/firmware.elf: $(LDSCRIPT) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware file"
	$(Q)$(OBJCOPY) -O binary $< $@
	$(ECHO) "`wc -c < $@` bytes"

include $(TOP)/py/mkrules.mk
