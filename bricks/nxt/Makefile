# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (c) 2019-2022 The Pybricks Authors

# ensure required git submodules checked out
ifeq ("$(wildcard ../../micropython/README.md)","")
$(info GIT cloning micropython submodule)
$(info $(shell cd ../.. && git submodule update --init micropython))
ifeq ("$(wildcard ../../micropython/README.md)","")
$(error failed)
endif
endif
ifeq ("$(wildcard ../../lib/nxos/README.md)","")
$(info GIT cloning nxos submodule)
$(info $(shell cd ../.. && git submodule update --init lib/nxos))
ifeq ("$(wildcard ../../lib/nxos/README.md)","")
$(error failed)
endif
endif

# lets micropython make files work with external files
USER_C_MODULES = ../..

include ../../micropython/py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = ../_common/qstrdefs.h
QSTR_GLOBAL_DEPENDENCIES =

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

MPY_CROSS = ../../micropython/mpy-cross/mpy-cross

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE ?= arm-none-eabi-

PBIO_PLATFORM = nxt

INC += -I.
INC += -I$(TOP)
INC += -I../../lib/contiki-core
INC += -I../../lib/lego
INC += -I../../lib/nxos/nxos
INC += -I../../lib/pbio/include
INC += -I../../lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I../../lib/pbio
INC += -I../..
INC += -I$(BUILD)

GIT = git
ZIP = zip
PYBRICKSDEV = pybricksdev
METADATA = ../../tools/metadata.py

COPT += -DFIXMATH_NO_CTYPE

CFLAGS_MCU = -mtune=arm7tdmi -mcpu=arm7tdmi -msoft-float
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU) $(COPT) $(CFLAGS_EXTRA)
LDSCRIPT = $(PBIO_PLATFORM).ld
LDFLAGS = -nostdlib -T $(LDSCRIPT) -Map=$@.map --cref --gc-sections

# avoid doubles
CFLAGS += -fsingle-precision-constant -Wdouble-promotion -DHAVE_STDBOOL_H

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

LIBS = "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

# Sources and libraries common to all pybricks bricks

include ../_common/sources.mk

# Embedded MicroPython sources

PY_EMBEDDED_SRC_C = $(addprefix bricks/,\
	_common/micropython.c \
	nxt/app.c \
	nxt/mphalport.c \
	)

# Extra core MicroPython files

# NB: Since we are using MicroPython's build system, files in the micropython/
# directory have the micropython/ prefix excluded. It is very important to do
# it that way since there is special handling of certain files that will break
# if we don't do it this way. So we need to be very careful about name clashes
# between the top level directory and the micropython/ subdirectory.

PY_EXTRA_SRC_C += $(addprefix shared/,\
	libc/string0.c \
	readline/readline.c \
	runtime/gchelper_native.c \
	runtime/interrupt_char.c \
	runtime/pyexec.c \
	runtime/stdout_helpers.c \
	runtime/sys_stdio_mphal.c \
	)

NXOS_SRC_C = $(addprefix lib/nxos/nxos/,\
	base/_abort.c \
	base/assert.c \
	base/display.c \
	base/drivers/_efc.c \
	base/drivers/_lcd.c \
	base/drivers/_twi.c \
	base/drivers/_uart.c \
	base/drivers/aic.c \
	base/drivers/avr.c \
	base/drivers/bt.c \
	base/drivers/i2c_memory.c \
	base/drivers/i2c.c \
	base/drivers/motors.c \
	base/drivers/radar.c \
	base/drivers/rs485.c \
	base/drivers/sensors.c \
	base/drivers/sound.c \
	base/drivers/systick.c \
	base/drivers/usb.c \
	base/lib/fs/fs.c \
	base/lib/gui/gui.c \
	base/lib/memalloc/memalloc.c \
	base/lib/rcmd/rcmd.c \
	base/lib/tracing/tracing.c \
	)

# Override nxos/base/util to use string.h to avoid conflict with MicroPython.
NXOS_EXT_SRC_C = $(addprefix bricks/nxt/,\
	base/util.c \
	)

SRC_S = \
	shared/runtime/gchelper_arm7tdmi.s \

SRC_S_NXOS = $(addprefix lib/nxos/nxos/,\
	base/init.s \
	base/interrupts.s \
	base/lock.s \
	base/samba_init.s \
	base/vectors.s \
	)

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(PY_EXTRA_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PY_EMBEDDED_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(NXOS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(NXOS_EXT_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S_NXOS:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(CONTIKI_SRC_C:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(PY_EXTRA_SRC_C) $(PY_EMBEDDED_SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

# Main firmware build targets
TARGETS := $(BUILD)/firmware.zip

all: $(TARGETS)

FW_VERSION := $(shell $(GIT) describe --tags --dirty --always --exclude "@pybricks/*")

$(BUILD)/firmware.elf: $(LDSCRIPT) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

# firmware blob with main.mpy - can be flashed to hub
$(BUILD)/firmware-base.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware file"
	$(Q)$(OBJCOPY) -O binary $^ $@
	$(ECHO) "`wc -c < $@` bytes"

# TODO: Add support for metadata
$(BUILD)/firmware.metadata.json: $(BUILD)/firmware.elf $(METADATA)
	$(ECHO) "META creating firmware metadata"
	$(Q)$(METADATA) $(FW_VERSION) $(PBIO_PLATFORM) $<.map $@

# firmware.zip file
ZIP_FILES := \
	$(BUILD)/firmware-base.bin \

$(BUILD)/firmware.zip: $(ZIP_FILES)
	$(ECHO) "ZIP creating firmware package"
	$(Q)$(ZIP) -j $@ $^

# TODO: Add support for pybricksdev
deploy: $(BUILD)/firmware.zip
	$(Q)$(PYBRICKSDEV) flash $< --name $(PBIO_PLATFORM)

include $(TOP)/py/mkrules.mk
