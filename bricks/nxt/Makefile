default: all

LEJOS_NXT_ARM_GCC ?= arm-none-eabi-gcc

BASE_ABI_FLAGS := -mcpu=arm7tdmi -mlittle-endian -mfloat-abi=soft -mthumb-interwork
LIB_ABI_FLAGS  := $(BASE_ABI_FLAGS) -mthumb

MACRO_PROG_PATH = $(abspath $(shell "$(CC)" -print-prog-name="$(1)"))

CC        := $(LEJOS_NXT_ARM_GCC)
OBJCOPY   := $(call MACRO_PROG_PATH,objcopy)
OBJDUMP   := $(call MACRO_PROG_PATH,objdump)

.PHONY: EnvironmentMessage
EnvironmentMessage:
	@echo " CC      $(CC)"
	@echo " OBJCOPY $(OBJCOPY)"
	@echo " OBJDUMP $(OBJDUMP)"
	@echo ""

TARGET := firmware.bin

C_PLATFORM_OBJECTS := $(addprefix nxt-firmware-drivers/nxt/,\
	aic.o \
	systick.o \
	udp.o \
	twi.o \
	nxt_spi.o \
	nxt_motors.o \
	data_abort.o \
	display.o \
	i2c.o \
	sound.o \
	bt.o \
	hs.o \
	usart.o \
	flashprog.o \
	flashwrite.o \
	printf.o \
	)

# merge main and mpy main
C_HOOK_OBJECTS := $(addprefix nxt-firmware-drivers/nxt/,\
	maininit.o \
	nxt_avr.o \
	sensors.o \
	nxt_lcd.o \
	)

C_OBJECTS := \
	$(C_PLATFORM_OBJECTS) \
	$(C_HOOK_OBJECTS) \
	$(C_RAM_OBJECTS)

ifeq ($(STANDALONE), 1)
	C_OBJECTS += $(addprefix nxt-firmware-drivers/nxt/,main.o)
endif

S_OBJECTS := $(addprefix nxt-firmware-drivers/nxt/,\
	interrupts.o \
	vectors.o \
	init.o \
	irq.o \
	)	

C_OPTIMISATION_FLAGS = -Os
#C_OPTIMISATION_FLAGS = -Os -Xassembler -aslh
#C_OPTIMISATION_FLAGS = -O0

CFLAGS_LEJOS = $(BASE_ABI_FLAGS) -mthumb \
	-ffreestanding -fsigned-char \
	$(C_OPTIMISATION_FLAGS) -g \
	-Wall -Winline -Werror-implicit-function-declaration \
	-Inxt-firmware-drivers/nxt \
	-ffunction-sections -fdata-sections

LDFLAGS_LEJOS = $(LIB_ABI_FLAGS) -nostdlib -nodefaultlibs -Wl,-cref,--gc-sections


ALL_BIN := $(TARGET)
ALL_OBJECTS := $(C_OBJECTS) $(S_OBJECTS)
ALL_ELF := $(ALL_BIN:.bin=.elf)
ALL_MAP := $(ALL_BIN:.bin=.map)
ALL_ASM := $(ALL_BIN:.bin=.asm) $(ALL_OBJECTS:.o=.asm)

.SECONDARY: $(ALL_ELF) $(ALL_MAP) $(ALL_OBJECTS)

.PHONY: all
all:  BuildMessage $(TARGET)

.PHONY: everything
everything: BuildMessage $(ALL_BIN) $(ALL_ASM)

.PHONY: TargetMessage
TargetMessage:
	@echo ""
	@echo "Building: $(TARGET)"
	@echo ""
	@echo "C objects: $(C_OBJECTS)"
	@echo ""
	@echo "Assembler objects: $(S_OBJECTS)"
	@echo ""
	@echo "LD source: $(LDSCRIPT_SOURCE)"
	@echo ""

.PHONY: BuildMessage
BuildMessage: TargetMessage EnvironmentMessage

.PHONY: clean
clean-standalone:
	@echo "Removing All Objects"
	@rm -f $(ALL_OBJECTS)
	@echo "Removing target"
	@rm -f $(ALL_ELF) $(ALL_BIN)
	@echo "Removing map files"
	@rm -f $(ALL_MAP)
	@echo "Removing asm files"
	@rm -f $(ALL_ASM)

firmware.elf: nxt-firmware-drivers/nxt/lejos_nxt_rom.lds $(C_OBJECTS) $(S_OBJECTS) mpobjects
	@echo "### Linking $@ using linker script $<"
	$(CC) $(LDFLAGS_LEJOS) -Wl,-T,$<,-Map,${@:.elf=.map} -o $@ $(C_OBJECTS) $(S_OBJECTS) $(OBJ) -lm -lc -lgcc

%.map: %.elf
	@true

%.bin: %.elf
	@echo "### Generating binary file $@ from $<"
	$(OBJCOPY) -O binary $< $@

# default rules for compiling sources

%.o: %.S
	@echo "### Assembling $< to $@"
	$(CC) $(CFLAGS_LEJOS) -c -o $@ $< 

%.o: %.c
	@echo "### Compiling $< to $@"
	$(CC) $(CFLAGS_LEJOS) -c -o $@ $< 


# MicroPython

include ../../../../py/mkenv.mk

CROSS = 1

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE = arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

CFLAGS_CORTEX_M4 = -mthumb -mtune=cortex-m4 -mabi=aapcs-linux -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant -Wdouble-promotion
CFLAGS = $(CFLAGS_LEJOS) $(INC) -Wall -Werror -std=c99 -nostdlib $(COPT)
LDFLAGS = $(LDFLAGS_LEJOS)

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else
CFLAGS += -Os -DNDEBUG
CFLAGS += -fdata-sections -ffunction-sections
endif

LIBS =

SRC_C = \
	main.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	$(BUILD)/_frozen_mpy.c \

OBJ = $(PY_CORE_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

$(BUILD)/_frozen_mpy.c: $(BUILD)/frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode" $(BUILD)
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/frozentest.mpy: frozentest.py
	$(Q)$(MPY_CROSS) -o $@ $(MPY_CROSS_FLAGS) $^

ifeq ($(STANDALONE), 1)
mpobjects:
else
mpobjects: $(OBJ) $(BUILD)/_frozen_mpy.c
endif

include $(TOP)/py/mkrules.mk
