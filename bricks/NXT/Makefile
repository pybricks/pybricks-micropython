# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George

include ../../../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE ?= arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

CFLAGS_ARM7TDMI = -mthumb -mtune=arm7tdmi -mcpu=arm7tdmi -mfloat-abi=soft
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_ARM7TDMI) $(COPT)

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
endif
CFLAGS += -fdata-sections -ffunction-sections

LDFLAGS = -nostdlib -T stm32f405.ld -Map=$@.map --cref --gc-sections
LIBS =

SRC_C = \
	main.c \
#	printf.c \
	string0.c \
	malloc0.c \
	gccollect.c \

SRC_S = \
#	startup_stm32f40xx.s \
	gchelper.s \

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))

all: $(BUILD)/firmware.elf

$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(Q)$(SIZE) $@

include $(TOP)/py/mkrules.mk
