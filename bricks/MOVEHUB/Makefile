include ../../../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE ?= arm-none-eabi-

INC += -I.
INC += -I$(TOP)
INC += -I$(TOP)/lib/cmsis/inc
INC += -I$(TOP)/lib/stm32lib/CMSIS/STM32F0xx/Include
INC += -I$(TOP)/ports/pybricks/lib/pbio/config/move_hub
INC += -I$(TOP)/ports/pybricks/lib/pbio/include
INC += -I$(BUILD)

DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CHECKSUM = $(TOP)/ports/pybricks/tools/checksum.py

CFLAGS_CORTEX_M0 = -mthumb -mtune=cortex-m0 -mcpu=cortex-m0  -msoft-float
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib $(CFLAGS_CORTEX_M0) $(COPT)
LDFLAGS = -nostdlib -T move-hub.ld -Map=$@.map --cref --gc-sections

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
	hub.c \
	modutime.c \
	accel.c \
	adc.c \
	button.c \
	systick.c \
	uart_core.c \
	lib/utils/interrupt_char.c \
	lib/utils/printf.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	ports/pybricks/lib/pbio/src/move_hub/light.c \
	ports/pybricks/lib/pbio/src/move_hub/motor.c \

SRC_S = \
	startup_stm32f070xb.s \
	ports/stm32/gchelper_m0.s \

SRC_LIBM = $(addprefix lib/libm/,\
	math.c \
	acoshf.c \
	asinfacosf.c \
	asinhf.c \
	atan2f.c \
	atanf.c \
	atanhf.c \
	ef_rem_pio2.c \
	erf_lgamma.c \
	fmodf.c \
	kf_cos.c \
	kf_rem_pio2.c \
	kf_sin.c \
	kf_tan.c \
	log1pf.c \
	nearbyintf.c \
	sf_cos.c \
	sf_erf.c \
	sf_frexp.c \
	sf_ldexp.c \
	sf_modf.c \
	sf_sin.c \
	sf_tan.c \
	wf_lgamma.c \
	wf_tgamma.c \
	ef_sqrt.c \
	)

# TODO: it looks like micropython has built-in makefiles for handling frozen files
FROZEN_C = \
	$(BUILD)/_frozen_mpy.c \

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o) $(FROZEN_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

all: $(BUILD)/firmware.bin

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

$(BUILD)/firmware-no-checksum.elf: $(OBJ)
	$(Q)$(LD) --defsym=CHECKSUM=0 $(LDFLAGS) -o $@ $^ $(LIBS)

$(BUILD)/firmware-no-checksum.bin: $(BUILD)/firmware-no-checksum.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $@

$(BUILD)/firmware.elf: $(BUILD)/firmware-no-checksum.bin $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) --defsym=CHECKSUM=`$(CHECKSUM) $<` $(LDFLAGS) -o $@ $(filter-out $<,$^) $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware file"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $@
	$(ECHO) "`stat --printf='%s\n' $@` bytes"


include $(TOP)/py/mkrules.mk
