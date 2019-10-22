# MicroPython

PBIO_PLATFORM = nxt

# Sanity check
include ../check.mk

include ../../../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_GLOBAL_DEPENDENCIES = $(TOP)/ports/pybricks/bricks/nxt/mpconfigport.h

# directory containing scripts to be frozen as bytecode
FROZEN_MPY_DIR ?= modules
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
INC += -I$(TOP)/ports/pybricks/lib/libfixmath/libfixmath
INC += -I$(TOP)/ports/pybricks/bricks/nxt/nxt-firmware-drivers
INC += -I$(TOP)/ports/pybricks/lib/pbio/include
INC += -I$(TOP)/ports/pybricks/lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I$(TOP)/ports/pybricks/lib/pbio
INC += -I$(TOP)/ports/pybricks/extmod
INC += -I$(BUILD)

COPT += -DFIXMATH_NO_CTYPE

CFLAGS_MCU = -mthumb -mtune=arm7tdmi -mcpu=arm7tdmi -msoft-float
CFLAGS = $(INC) -Werror -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU) $(COPT)
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

ifneq ($(FROZEN_MPY_DIR),)
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
endif

LIBS = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

SRC_C = $(addprefix ports/pybricks/bricks/nxt/,\
	main.c \
	systick.c \
	)

SRC_C += \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	lib/utils/interrupt_char.c \
	lib/utils/pyexec.c \
	lib/utils/stdout_helpers.c \

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
PYBRICKS_PY_SRC_C = $(addprefix ports/pybricks/extmod/,\
	modadvanced.c \
	modbattery.c \
	modlogger.c \
	modmotor.c \
	modnxtdevices.c \
	modparameters.c \
	modrobotics.c \
	modtools.c \
	pberror.c \
	pbhub.c \
	pbobj.c \
	)


LIBFIXMATH_SRC_C = $(addprefix ports/pybricks/lib/libfixmath/libfixmath/,\
	fix16_sqrt.c \
	fix16_str.c \
	fix16.c \
	uint32.c \
	)

PBIO_SRC_C = $(addprefix ports/pybricks/lib/pbio/,\
	drv/$(PBIO_PLATFORM)/motor.c \
	drv/battery/battery_nxt.c \
	drv/button/button_nxt.c \
	drv/counter/counter_core.c \
	drv/counter/counter_nxt.c \
	platform/$(PBIO_PLATFORM)/clock.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	platform/$(PBIO_PLATFORM)/sys.c \
	src/control.c \
	src/drivebase.c \
	src/error.c \
	src/fixmath.c \
	src/hbridge.c \
	src/logger.c \
	src/main.c \
	src/servo.c \
	src/tacho.c \
	src/trajectory.c \
	sys/autostart.c \
	sys/etimer.c \
	sys/process.c \
	sys/timer.c \
	)

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C_NXT:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S_NXT:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PY_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIBFIXMATH_SRC_C:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(PYBRICKS_PY_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

all: $(BUILD)/firmware.bin

$(BUILD)/firmware.elf: $(LDSCRIPT) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware file"
	$(OBJCOPY) -O binary $< $@
	$(ECHO) "`wc -c < $@` bytes"

include $(TOP)/py/mkrules.mk
