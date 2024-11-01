# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (c) 2019-2022 The Pybricks Authors

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
PBTOP := $(patsubst %/bricks/ev3rt/Makefile,%,$(mkfile_path))

# ensure required git submodules checked out
ifeq ("$(wildcard $(PBTOP)/micropython/README.md)","")
$(info GIT cloning micropython submodule)
$(info $(shell cd $(PBTOP) && git submodule update --init micropython))
ifeq ("$(wildcard $(PBTOP)/micropython/README.md)","")
$(error failed)
endif
endif
ifeq ("$(wildcard $(PBTOP)/lib/ev3rt-lib/modules/mbed-on-toppers/README.md)","")
$(info GIT cloning ev3rt-lib submodule)
$(info $(shell cd $(PBTOP) && git submodule update --checkout --init --recursive lib/ev3rt-lib))
ifeq ("$(wildcard $(PBTOP)/lib/ev3rt-lib/README.md)","")
$(error failed)
endif
endif

# lets micropython make files work with external files
USER_C_MODULES = $(PBTOP)

include ../../micropython/py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = ../_common/qstrdefs.h
QSTR_GLOBAL_DEPENDENCIES = ../_common/mpconfigport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

CROSS_COMPILE ?= arm-none-eabi-

PBIO_PLATFORM = ev3rt

INC += -I.
INC += -I$(TOP)
INC += -I$(PBTOP)/lib/contiki-core
INC += -I$(PBTOP)/lib/lego
INC += -I$(PBTOP)/lib/lwrb/src/include
INC += -I$(PBTOP)/lib/pbio/include
INC += -I$(PBTOP)/lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I$(PBTOP)/lib/pbio
INC += -I$(PBTOP)/lib/ev3rt-lib
INC += -I$(PBTOP)/lib/ev3rt-lib/arch/arm_gcc/am1808
INC += -I$(PBTOP)/lib/ev3rt-lib/arch/arm_gcc/common
INC += -I$(PBTOP)/lib/ev3rt-lib/arch/gcc
INC += -I$(PBTOP)/lib/ev3rt-lib/include
INC += -I$(PBTOP)/lib/ev3rt-lib/sdk/common
INC += -I$(PBTOP)/lib/ev3rt-lib/sdk/common/ev3api/include
INC += -I$(PBTOP)/lib/ev3rt-lib/sdk/common/ev3api/src
INC += -I$(PBTOP)/lib/ev3rt-lib/target/ev3_gcc
INC += -I$(PBTOP)/lib/ev3rt-lib/target/ev3_gcc/pil/include

INC += -I$(PBTOP)
INC += -I$(BUILD)

GIT = git
ZIP = zip
PYBRICKSDEV = pybricksdev
METADATA = $(PBTOP)/tools/metadata.py

CFLAGS_MCU =
CFLAGS_WARN = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-maybe-uninitialized
CFLAGS = $(INC) -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU) $(CFLAGS_WARN) $(COPT) $(CFLAGS_EXTRA)
$(BUILD)/lib/libm/%.o: CFLAGS += -Wno-sign-compare

LDFLAGS = $(addprefix -T,$(LD_FILES)) -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections

# avoid doubles
CFLAGS += -fsingle-precision-constant -Wdouble-promotion

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -Og -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

MPY_CROSS = ../../micropython/mpy-cross/mpy-cross

LIBS = "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

CFLAGS += -DTOPPERS_OMIT_TECS

# Sources and libraries common to all pybricks bricks

include ../_common/sources.mk

# Embedded MicroPython sources

PY_EMBEDDED_SRC_C = $(addprefix bricks/,\
	_common/micropython.c \
	ev3rt/mphalport.c \
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

SRC_S = shared/runtime/gchelper_thumb1.s

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(PY_EXTRA_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PY_EMBEDDED_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(CONTIKI_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LWRB_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(PY_EXTRA_SRC_C) $(PY_EMBEDDED_SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

FW_VERSION := $(shell $(GIT) describe --tags --dirty --always --exclude "@pybricks/*")

$(BUILD)/pybricks.cfg: clear-cfg
	$(ECHO) "Creating $@"
	$(Q)$(PYTHON) make_config.py $@ $(OBJ)

clear-cfg:
	rm -f $(BUILD)/pybricks.cfg

include $(TOP)/py/mkrules.mk

#
# Paths and object files
#
KERNEL := hrp
EV3RT_BRICK_DIR := $(PBTOP)/bricks/ev3rt
EV3RT_LIB_DIR := $(PBTOP)/lib/ev3rt-lib
APPLDIRS := \
	$(EV3RT_BRICK_DIR) \
	$(PBTOP)/lib/pbio/include \
	$(PBTOP)/lib/pbio/platform/ev3rt \
	$(PBTOP)/micropython/py \

#
# Used by sub-makefile
#
export EV3RT_BRICK_DIR
export EV3RT_LIB_DIR
export APPLDIRS

all: uImage firmware

firmware: $(BUILD)/firmware.zip

ev3rt-config: $(EV3RT_BRICK_DIR) $(BUILD)/libkernel.a prepare-obj-folder
	make -C $(BUILD) offset.h kernel_cfg.h

mkimage: $(OBJ)
	make -C $(BUILD) > /dev/null && \
	arm-none-eabi-objcopy -O binary $(BUILD)/$(KERNEL) $(BUILD)/$(KERNEL).bin
	@mkimage -A arm -O linux -T kernel -C none -a 0xc0008000 -e 0xc0008000 \
        -n "TOPPERS/$(KERNEL) Kernel (EV3)" \
		-d $(BUILD)/$(KERNEL).bin $(BUILD)/uImage
	@chmod +x $(BUILD)/uImage

uImage:
	mkdir -p $(EV3RT_BRICK_DIR)/build
	make $(BUILD)/pybricks.cfg
	make ev3rt-config
	make mkimage
$(BUILD)/libkernel.a: prepare-obj-folder
	make -C $(BUILD) libkernel.a

prepare-obj-folder:
	mkdir -p $(EV3RT_BRICK_DIR)/build
	mkdir -p $(EV3RT_BRICK_DIR)/build/objs
	cp $(EV3RT_BRICK_DIR)/ev3rt.mk $(EV3RT_BRICK_DIR)/build/Makefile

#TODO: Build our own u-boot image. Use any official firmware for now. u-boot will be extracted from it.
UBOOT_FILE = $(EV3RT_BRICK_DIR)/officialfw.bin

$(BUILD)/firmware-base.bin: uImage
	python $(EV3RT_BRICK_DIR)/make_bootable_image.py $(UBOOT_FILE) $(BUILD)/uImage $(BUILD)/firmware-base.bin

$(BUILD)/firmware.metadata.json: $(BUILD)/firmware-base.bin $(METADATA)
	$(ECHO) "META creating firmware metadata"
	$(Q)$(METADATA) $(FW_VERSION) $(PBIO_PLATFORM) $< $@

ZIP_FILES := \
	$(BUILD)/firmware-base.bin \
	$(BUILD)/firmware.metadata.json \
	ReadMe_OSS.txt \

$(BUILD)/firmware.zip: $(ZIP_FILES)
	$(ECHO) "ZIP creating firmware package"
	$(Q)$(ZIP) -j $@ $^

deploy: $(BUILD)/firmware.zip
	$(Q)$(PYBRICKSDEV) flash $<
