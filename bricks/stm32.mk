# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (C) 2019 David Lechner <david@pybricks.com>

# This file is shared by all STM32-based Pybricks ports
# Other ports should not use this file

# Sanity check
include ../check.mk

include ../../../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_GLOBAL_DEPENDENCIES = $(TOP)/ports/pybricks/bricks/stm32configport.h

# Module included as mpy file in persistent user ROM
# PYBRICKS_MPY_MAIN_MODULE ?= main.py

# directory containing scripts to be frozen as bytecode
FROZEN_MPY_DIR ?= modules
FROZEN_MPY_TOOL_ARGS = -mlongint-impl=none

# include py core make definitions
include $(TOP)/py/py.mk

CROSS_COMPILE ?= arm-none-eabi-

# Bricks must specify the following variables in their Makefile
ifeq ($(PB_MCU_SERIES),)
$(error "PB_MCU_SERIES is not specified - add it in <hub>/Makefile)
else
PB_MCU_SERIES_LCASE = $(subst F,f,$(subst L,l,$(PB_MCU_SERIES)))
endif
ifeq ($(PB_CMSIS_MCU),)
$(error "PB_CMSIS_MCU is not specified - add it in <hub>/Makefile")
endif
ifeq ($(PBIO_PLATFORM),)
$(error "PBIO_PLATFORM is not specified - add it in <hub>/Makefile)
endif

INC += -I.
INC += -I$(TOP)
INC += -I$(TOP)/lib/cmsis/inc
INC += -I$(TOP)/lib/stm32lib/CMSIS/STM32$(PB_MCU_SERIES)xx/Include
ifeq ($(PB_USE_HAL),1)
INC += -I$(TOP)/lib/stm32lib/STM32$(PB_MCU_SERIES)xx_HAL_Driver/Inc
endif
INC += -I$(TOP)/ports/pybricks/lib/contiki-core
INC += -I$(TOP)/ports/pybricks/lib/libfixmath/libfixmath
INC += -I$(TOP)/ports/pybricks/lib/pbio/include
INC += -I$(TOP)/ports/pybricks/lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I$(TOP)/ports/pybricks/lib/pbio
INC += -I$(TOP)/ports/pybricks/lib/BlueNRG-MS/includes
INC += -I$(TOP)/ports/pybricks/extmod
INC += -I$(TOP)/ports/pybricks/py
INC += -I$(BUILD)

DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CHECKSUM = $(TOP)/ports/pybricks/tools/checksum.py
CHECKSUM_TYPE ?= xor
OPENOCD ?= openocd
OPENOCD_CONFIG ?= openocd_stm32$(PB_MCU_SERIES_LCASE).cfg
TEXT0_ADDR ?= 0x08000000

COPT += -DFIXMATH_NO_CTYPE

CFLAGS_MCU_F0 = -mthumb -mtune=cortex-m0 -mcpu=cortex-m0  -msoft-float
CFLAGS_MCU_F4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU_L4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU_$(PB_MCU_SERIES)) $(COPT)
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

# Required for STM32 library
CFLAGS += -D$(PB_CMSIS_MCU)

CFLAGS += -DSTM32_HAL_H='<stm32$(PB_MCU_SERIES_LCASE)xx_hal.h>'

ifneq ($(PYBRICKS_MPY_MAIN_MODULE),)
CFLAGS += -DPYBRICKS_MPY_MAIN_MODULE
endif

ifneq ($(FROZEN_MPY_DIR),)
# To use frozen bytecode, put your .py files in a subdirectory (eg frozen/) and
# then invoke make with FROZEN_MPY_DIR=frozen (be sure to build from scratch).
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
endif

LIBS = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

SRC_C = $(addprefix ports/pybricks/bricks/,\
	stm32_main.c \
	systick.c \
	uart_core.c \
	)

SRC_C += \
	lib/libc/string0.c \
	lib/mp-readline/readline.c \
	lib/utils/interrupt_char.c \
	lib/utils/pyexec.c \
	lib/utils/stdout_helpers.c \

SRC_S = \
	ports/pybricks/lib/pbio/platform/$(PBIO_PLATFORM)/startup.s \

ifeq ($(PB_MCU_SERIES),F0)
	SRC_S += $(TOP)/lib/utils/gchelper_m0.s
else
	SRC_S += $(TOP)/lib/utils/gchelper_m0.s
endif

# Pybricks modules
PYBRICKS_EXTMOD_SRC_C = $(addprefix ports/pybricks/extmod/,\
	modadvanced.c \
	modbattery.c \
	moddebug.c \
	modlight.c \
	pblight.c \
	modmotor.c \
	modhubs.c \
	modparameters.c \
	modpupdevices.c \
	modtools.c \
	modlogger.c \
	modrobotics.c \
	pbhub.c \
	pbiodevice.c \
	)

PYBRICKS_PY_SRC_C = $(addprefix ports/pybricks/py/,\
	modenum.c \
	pbobj.c \
	pberror.c \
	)

BLUENRG_SRC_C = $(addprefix ports/pybricks/lib/BlueNRG-MS/hci/,\
	controller/bluenrg_gap_aci.c \
	controller/bluenrg_gatt_aci.c \
	controller/bluenrg_hal_aci.c \
	controller/bluenrg_l2cap_aci.c \
	controller/bluenrg_updater_aci.c \
	hci_le.c \
	)

HAL_SRC_C = $(addprefix lib/stm32lib/STM32$(PB_MCU_SERIES)xx_HAL_Driver/Src/,\
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_cortex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dma.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_gpio.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pwr_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_rcc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_tim_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_tim.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_uart_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_uart.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal.c \
	)

CONTIKI_SRC_C = $(addprefix ports/pybricks/lib/contiki-core/,\
	sys/autostart.c \
	sys/etimer.c \
	sys/process.c \
	sys/timer.c \
	)

LIBFIXMATH_SRC_C = $(addprefix ports/pybricks/lib/libfixmath/libfixmath/,\
	fix16_sqrt.c \
	fix16_str.c \
	fix16.c \
	uint32.c \
	)

PBIO_SRC_C = $(addprefix ports/pybricks/lib/pbio/,\
	drv/$(PBIO_PLATFORM)/bluetooth.c \
	drv/$(PBIO_PLATFORM)/light.c \
	drv/$(PBIO_PLATFORM)/motor.c \
	drv/adc/adc_stm32f0.c \
	drv/adc/adc_stm32_hal.c \
	drv/battery/battery_adc.c \
	drv/button/button_gpio.c \
	drv/counter/counter_core.c \
	drv/counter/counter_stm32f0_gpio_quad_enc.c \
	drv/gpio/gpio_stm32f0.c \
	drv/gpio/gpio_stm32f4.c \
	drv/gpio/gpio_stm32l4.c \
	drv/ioport/ioport_lpf2.c \
	drv/uart/uart_stm32f0.c \
	drv/uart/uart_stm32_hal.c \
	platform/$(PBIO_PLATFORM)/clock.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	platform/$(PBIO_PLATFORM)/sys.c \
	src/error.c \
	src/iodev.c \
	src/light.c \
	src/main.c \
	src/hbridge.c \
	src/tacho.c \
	src/logger.c \
	src/servo.c \
	src/drivebase.c \
	src/control.c \
	src/trajectory.c \
	src/fixmath.c \
	src/uartdev.c \
	)

SRC_LIBM = $(addprefix lib/libm/,\
	acoshf.c \
	asinfacosf.c \
	asinhf.c \
	atan2f.c \
	atanf.c \
	atanhf.c \
	ef_rem_pio2.c \
	ef_sqrt.c \
	erf_lgamma.c \
	fmodf.c \
	kf_cos.c \
	kf_rem_pio2.c \
	kf_sin.c \
	kf_tan.c \
	log1pf.c \
	math.c \
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
	)

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_EXTMOD_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PY_SRC_C:.c=.o))
ifeq ($(PB_LIB_BLUENRG),1)
OBJ += $(addprefix $(BUILD)/, $(BLUENRG_SRC_C:.c=.o))
endif
ifeq ($(PB_USE_HAL),1)
OBJ += $(addprefix $(BUILD)/, $(HAL_SRC_C:.c=.o))
endif
OBJ += $(addprefix $(BUILD)/, $(CONTIKI_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIBFIXMATH_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))

# Optionally append .mpy file specified by PYBRICKS_MPY_MAIN_MODULE to 2K free space after 106K firmware
ifneq ($(PYBRICKS_MPY_MAIN_MODULE),)
OBJ += $(BUILD)/main_mpy.o

MPY_CROSS_FLAGS += -mno-unicode

$(BUILD)/main.mpy: $(PYBRICKS_MPY_MAIN_MODULE)
	$(Q)$(MPY_CROSS) -o $@ $(MPY_CROSS_FLAGS)  $^

$(BUILD)/main_mpy.o: $(BUILD)/main.mpy
	$(Q)$(OBJCOPY) -I binary -O elf32-littlearm -B arm --rename-section .data=.mpy,alloc,load,readonly,data,contents $^ $@

MPYSIZE := $$(wc -c < "$(BUILD)/main.mpy")

FIRMWARE_EXTRA_ARGS = -j .user --gap-fill=0xff
else
MPYSIZE := 0
endif

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(PYBRICKS_PY_SRC_C) $(PYBRICKS_EXTMOD_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

all: $(BUILD)/firmware.bin

$(BUILD)/firmware-no-checksum.elf: $(LDSCRIPT) $(OBJ)
	$(Q)$(LD) --defsym=MPYSIZE=$(MPYSIZE) --defsym=CHECKSUM=0 $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

$(BUILD)/firmware-no-checksum.bin: $(BUILD)/firmware-no-checksum.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data -j .checksum $^ $@

$(BUILD)/firmware.elf: $(BUILD)/firmware-no-checksum.bin $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) --defsym=MPYSIZE=$(MPYSIZE) --defsym=CHECKSUM=`$(CHECKSUM) $(CHECKSUM_TYPE) $< $(PB_FIRMWARE_MAX_SIZE)` $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) $@

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware file"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data -j .checksum $(FIRMWARE_EXTRA_ARGS) $^ $@
	$(ECHO) "`wc -c < $@` bytes"

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(DFU) -b $(TEXT0_ADDR):$< $@

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(PYDFU) -u $<

deploy-openocd: $(BUILD)/firmware-no-checksum.bin
	$(ECHO) "Writing $< to the board via ST-LINK using OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "stm_flash $< $(TEXT0_ADDR)"

include $(TOP)/py/mkrules.mk
