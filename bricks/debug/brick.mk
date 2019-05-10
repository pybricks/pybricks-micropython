# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George

include ../../../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_GLOBAL_DEPENDENCIES = mpconfigbrick.h

#PYBRICKS_MPY_MAIN_MODULE ?= modules/main.py

# directory containing scripts to be frozen as bytecode
FROZEN_MPY_DIR ?= modules
FROZEN_MPY_TOOL_ARGS = -mlongint-impl=none

# include py core make definitions
include $(TOP)/py/py.mk

OPENOCD ?= openocd
OPENOCD_CONFIG ?= openocd_stm32f4.cfg
TEXT0_ADDR ?= 0x08000000

CROSS_COMPILE ?= arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(TOP)/lib/cmsis/inc
INC += -I$(TOP)/lib/stm32lib/CMSIS/STM32F4xx/Include
INC += -I$(BUILD)

DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CFLAGS_CORTEX_M4 = -mthumb -mtune=cortex-m4 -mabi=aapcs-linux -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant -Wdouble-promotion
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_CORTEX_M4) $(COPT)
LDFLAGS = -nostdlib -T stm32f446.ld -Map=$@.map --cref --gc-sections

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

ifneq ($(PYBRICKS_MPY_MAIN_MODULE),)
CFLAGS += -DPYBRICKS_MPY_MAIN_MODULE=MP_STRINGIFY\($(basename $(notdir $(PYBRICKS_MPY_MAIN_MODULE)))\)
endif

ifneq ($(FROZEN_MPY_DIR),)
# To use frozen bytecode, put your .py files in a subdirectory (eg frozen/) and
# then invoke make with FROZEN_MPY_DIR=frozen (be sure to build from scratch).
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
endif

LIBS =

SRC_C = \
	main.c \
	uart_core.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \

SRC_S = \
	$(TOP)/ports/stm32/boards/startup_stm32f4.s \
	$(TOP)/ports/stm32/gchelper.s \

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))

# Optionally append .mpy file specified by PYBRICKS_MPY_MAIN_MODULE to 2K free space after 106K firmware
ifneq ($(PYBRICKS_MPY_MAIN_MODULE),)
OBJ += $(BUILD)/main_mpy.o

$(BUILD)/main.mpy: $(PYBRICKS_MPY_MAIN_MODULE)
	$(Q)$(MPY_CROSS) -o $@ $(MPY_CROSS_FLAGS) $^

$(BUILD)/main_mpy.o: $(BUILD)/main.mpy
	$(Q)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.mpy,alloc,load,readonly,data,contents $^ $@

FIRMWARE_EXTRA_ARGS = -j .user --gap-fill=0xff
endif

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(PYBRICKS_DRIVERS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

all: $(BUILD)/firmware.dfu

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $(BUILD)/firmware.bin

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(DFU) -b 0x08000000:$(BUILD)/firmware.bin $@

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(PYDFU) -u $<

deploy-openocd: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $(BUILD)/firmware.bin to the board via ST-LINK using OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "stm_flash $(BUILD)/firmware.bin $(TEXT0_ADDR)"

test: $(BUILD)/firmware.elf
	$(Q)/bin/echo -e "print('hello world!', list(x+1 for x in range(10)), end='eol\\\\n')\\r\\n\\x04" | $(BUILD)/firmware.elf | tail -n2 | grep "^hello world! \\[1, 2, 3, 4, 5, 6, 7, 8, 9, 10\\]eol"

include $(TOP)/py/mkrules.mk
