# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (C) 2019-2020 The Pybricks Authors

# This file is shared by all STM32-based Pybricks ports
# Other ports should not use this file

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
PBTOP := ../$(patsubst %/stm32/stm32.mk,%,$(THIS_MAKEFILE))

# ensure required git submodules checked out
ifeq ("$(wildcard $(PBTOP)/micropython/README.md)","")
$(info GIT cloning micropython submodule)
$(info $(shell cd $(PBTOP) && git submodule update --init micropython))
ifeq ("$(wildcard $(PBTOP)/micropython/README.md)","")
$(error failed)
endif
endif
ifeq ("$(wildcard $(PBTOP)/micropython/lib/stm32lib/README.md)","")
$(info GIT cloning stm32lib submodule)
$(info $(shell cd $(PBTOP)/micropython && git submodule update --init lib/stm32lib))
ifeq ("$(wildcard $(PBTOP)/micropython/lib/stm32lib/README.md)","")
$(error failed)
endif
endif
ifeq ("$(wildcard $(PBTOP)/lib/libfixmath/README.md)","")
$(info GIT cloning libfixmath submodule)
$(info $(shell cd $(PBTOP) && git submodule update --init lib/libfixmath))
ifeq ("$(wildcard $(PBTOP)/lib/libfixmath/README.md)","")
$(error failed)
endif
endif

# lets micropython make files work with external files
USER_C_MODULES = $(PBTOP)

include ../../micropython/py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_GLOBAL_DEPENDENCIES = $(PBTOP)/bricks/stm32/configport.h

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
INC += -I$(PBTOP)/lib/contiki-core
INC += -I$(PBTOP)/lib/lego
INC += -I$(PBTOP)/lib/libfixmath/libfixmath
INC += -I$(PBTOP)/lib/pbio/include
INC += -I$(PBTOP)/lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I$(PBTOP)/lib/pbio
ifeq ($(PB_LIB_BLUENRG),1)
INC += -I$(PBTOP)/lib/BlueNRG-MS/includes
endif
ifeq ($(PB_LIB_BLE5STACK),1)
INC += -I$(PBTOP)/lib/ble5stack/central
endif
ifeq ($(PB_USE_LSM6DS3TR_C),1)
INC += -I$(PBTOP)/lib/lsm6ds3tr_c_STdC/driver
endif
ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
INC += -I$(PBTOP)/lib/STM32_USB_Device_Library/Class/CDC/Inc/
INC += -I$(PBTOP)/lib/STM32_USB_Device_Library/Core/Inc/
endif
INC += -I$(PBTOP)/extmod
INC += -I$(PBTOP)/pybricks
INC += -I$(PBTOP)/py
INC += -I$(BUILD)

GIT = git
ZIP = zip
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
CHECKSUM = $(PBTOP)/tools/checksum.py
CHECKSUM_TYPE ?= xor
METADATA = $(PBTOP)/tools/metadata.py
OPENOCD ?= openocd
OPENOCD_CONFIG ?= openocd_stm32$(PB_MCU_SERIES_LCASE).cfg
TEXT0_ADDR ?= 0x08000000

CFLAGS_MCU_F0 = -mthumb -mtune=cortex-m0 -mcpu=cortex-m0  -msoft-float
CFLAGS_MCU_F4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU_L4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU_$(PB_MCU_SERIES)) $(COPT)

# define external oscillator frequency
CFLAGS += -DHSE_VALUE=$(PB_MCU_EXT_OSC_HZ)

# linker scripts
LD_FILES = $(PBIO_PLATFORM).ld
# not all hubs share common script
ifeq ($(filter $(PBIO_PLATFORM),debug prime_hub),)
LD_FILES += $(PBTOP)/bricks/stm32/common.ld
endif

LDFLAGS = -nostdlib $(addprefix -T,$(LD_FILES)) -Map=$@.map --cref --gc-sections

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

CFLAGS += -DSTM32_H='<stm32$(PB_MCU_SERIES_LCASE)xx.h>'
CFLAGS += -DSTM32_HAL_H='<stm32$(PB_MCU_SERIES_LCASE)xx_hal.h>'

# TODO: probably only need no-unicode on movehub
MPY_CROSS_FLAGS += -mno-unicode


LIBS = $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

SRC_C = $(addprefix bricks/stm32/,\
	main.c \
	pbdevice.c \
	systick.c \
	uart_core.c \
	)

SRC_C += $(addprefix micropython/lib/,\
	libc/string0.c \
	mp-readline/readline.c \
	utils/interrupt_char.c \
	utils/pyexec.c \
	utils/stdout_helpers.c \
	)

SRC_S = \
	lib/pbio/platform/$(PBIO_PLATFORM)/startup.s \

ifeq ($(PB_MCU_SERIES),F0)
	SRC_S += micropython/lib/utils/gchelper_m0.s
else
	SRC_S += micropython/lib/utils/gchelper_m0.s
endif

# Pybricks modules

PYBRICKS_PYBRICKS_SRC_C = $(addprefix pybricks/,\
    pybricks.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_drivebase.c \
	tools/pb_module_tools.c \
	tools/pb_type_stopwatch.c \
	)

PYBRICKS_EXTMOD_SRC_C = $(addprefix extmod/,\
	modbattery.c \
	modbuiltins.c \
	modbuttons.c \
	moddebug.c \
	modexperimental.c \
	modhubs.c \
	modiodevices.c \
	modlogger.c \
	modmotor.c \
	modparameters.c \
	modpupdevices.c \
	moduos.c \
	pbhsv.c \
	pb_type_matrix.c \
	)

# Pybricks helpers

PYBRICKS_PY_SRC_C = $(addprefix py/,\
	pb_type_enum.c \
	pberror.c \
	pbobj.c \
	)

# STM32 Bluetooth stack

BLUENRG_SRC_C = $(addprefix lib/BlueNRG-MS/hci/,\
	controller/bluenrg_gap_aci.c \
	controller/bluenrg_gatt_aci.c \
	controller/bluenrg_hal_aci.c \
	controller/bluenrg_l2cap_aci.c \
	controller/bluenrg_updater_aci.c \
	hci_le.c \
	)

# TI Bluetooth stack

BLE5STACK_SRC_C = $(addprefix lib/ble5stack/central/,\
	att.c \
	gap.c \
	gatt.c \
	hci_ext.c \
	hci.c \
	util.c \
	)

# Contiki

CONTIKI_SRC_C = $(addprefix lib/contiki-core/,\
	lib/ringbuf.c \
	sys/autostart.c \
	sys/etimer.c \
	sys/process.c \
	sys/timer.c \
	)

# STM32

COPT += -DUSE_FULL_LL_DRIVER

HAL_SRC_C = $(addprefix micropython/lib/stm32lib/STM32$(PB_MCU_SERIES)xx_HAL_Driver/Src/,\
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_cortex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dma.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_gpio.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_i2c.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pcd_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pcd.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pwr_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_rcc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_spi.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_tim_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_tim.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_uart_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_uart.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_ll_lpuart.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_ll_rcc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_ll_usart.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_ll_usb.c \
	)

ifeq ($(PB_MCU_SERIES),F4)
HAL_SRC_C := $(filter-out %xx_hal_uart_ex.c, $(HAL_SRC_C))
endif
ifneq ($(PB_MCU_SERIES),L4)
HAL_SRC_C := $(filter-out %xx_ll_lpuart.c, $(HAL_SRC_C))
endif
ifneq ($(PB_LIB_STM32_USB_DEVICE),1)
HAL_SRC_C := $(filter-out %xx_hal_pcd_ex.c, $(HAL_SRC_C))
HAL_SRC_C := $(filter-out %xx_hal_pcd.c, $(HAL_SRC_C))
HAL_SRC_C := $(filter-out %xx_ll_usb.c, $(HAL_SRC_C))
endif
# libfixmath

COPT += -DFIXMATH_NO_CTYPE

LIBFIXMATH_SRC_C = $(addprefix lib/libfixmath/libfixmath/,\
	fix16_sqrt.c \
	fix16_str.c \
	fix16.c \
	uint32.c \
	)

# Pybricks I/O library

PBIO_SRC_C = $(addprefix lib/pbio/,\
	drv/$(PBIO_PLATFORM)/bluetooth.c \
	drv/$(PBIO_PLATFORM)/light.c \
	drv/$(PBIO_PLATFORM)/motor.c \
	drv/adc/adc_stm32_hal.c \
	drv/adc/adc_stm32f0.c \
	drv/battery/battery_adc.c \
	drv/button/button_adc.c \
	drv/button/button_gpio.c \
	drv/core.c \
	drv/counter/counter_core.c \
	drv/counter/counter_stm32f0_gpio_quad_enc.c \
	drv/gpio/gpio_stm32f0.c \
	drv/gpio/gpio_stm32f4.c \
	drv/gpio/gpio_stm32l4.c \
	drv/ioport/ioport_lpf2.c \
	drv/pwm/pwm_core.c \
	drv/pwm/pwm_stm32_tim.c \
	drv/uart/uart_stm32f0.c \
	drv/uart/uart_stm32f4_ll_irq.c \
	drv/uart/uart_stm32l4_ll_dma.c \
	drv/usb/stm32_usb_serial.c \
	platform/$(PBIO_PLATFORM)/clock.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	platform/$(PBIO_PLATFORM)/sys.c \
	src/color/conversion.c \
	src/control.c \
	src/dcmotor.c \
	src/drivebase.c \
	src/error.c \
	src/integrator.c \
	src/iodev.c \
	src/light.c \
	src/logger.c \
	src/main.c \
	src/math.c \
	src/motorpoll.c \
	src/servo.c \
	src/tacho.c \
	src/trajectory_ext.c \
	src/trajectory.c \
	src/uartdev.c \
	)

# STM32 IMU Library

LSM6DS3TR_C_SRC_C = lib/lsm6ds3tr_c_STdC/driver/lsm6ds3tr_c_reg.c

# MicroPython math library

SRC_LIBM = $(addprefix micropython/lib/libm/,\
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

# STM32 USB Device library

SRC_STM32_USB_DEV = $(addprefix lib/STM32_USB_Device_Library/,\
	Class/CDC/Src/usbd_cdc.c \
	Core/Src/usbd_core.c \
	Core/Src/usbd_ctlreq.c \
	Core/Src/usbd_ioreq.c \
	)

SRC_STM32_USB_DEV += $(addprefix lib/pbio/platform/$(PBIO_PLATFORM)/,\
	usbd_conf.c \
	usbd_desc.c \
	)

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o) $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_EXTMOD_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PY_SRC_C:.c=.o))
ifeq ($(PB_LIB_BLUENRG),1)
OBJ += $(addprefix $(BUILD)/, $(BLUENRG_SRC_C:.c=.o))
endif
ifeq ($(PB_LIB_BLE5STACK),1)
OBJ += $(addprefix $(BUILD)/, $(BLE5STACK_SRC_C:.c=.o))
endif
ifeq ($(PB_USE_HAL),1)
OBJ += $(addprefix $(BUILD)/, $(HAL_SRC_C:.c=.o))
endif
ifeq ($(PB_USE_LSM6DS3TR_C),1)
OBJ += $(addprefix $(BUILD)/, $(LSM6DS3TR_C_SRC_C:.c=.o))
endif
OBJ += $(addprefix $(BUILD)/, $(CONTIKI_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIBFIXMATH_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))
ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_STM32_USB_DEV:.c=.o))
endif
OBJ += $(BUILD)/main.mpy.o

$(BUILD)/main.mpy: main.py
	$(ECHO) "MPY $<"
	$(Q)$(MPY_CROSS) -o $@ $(MPY_CROSS_FLAGS) $<
	$(ECHO) "`wc -c < $@` bytes"

$(BUILD)/main.mpy.o: $(BUILD)/main.mpy
	$(Q)$(OBJCOPY) -I binary -O elf32-littlearm -B arm \
		--rename-section .data=.mpy,alloc,load,readonly,data,contents $^ $@

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(PYBRICKS_PY_SRC_C) $(PYBRICKS_EXTMOD_SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

all: $(BUILD)/firmware.zip

FW_CHECKSUM := $$($(CHECKSUM) $(CHECKSUM_TYPE) $(BUILD)/firmware-no-checksum.bin $(PB_FIRMWARE_MAX_SIZE))
FW_VERSION := $(shell $(GIT) describe --tags --dirty --always)

$(BUILD)/firmware-no-checksum.elf: $(LD_FILES) $(OBJ)
	$(Q)$(LD) --defsym=CHECKSUM=0 $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

# firmware blob used to calculate checksum
$(BUILD)/firmware-no-checksum.bin: $(BUILD)/firmware-no-checksum.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data -j .user -j .checksum $^ $@

$(BUILD)/firmware.elf: $(BUILD)/firmware-no-checksum.bin $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) --defsym=CHECKSUM=$(FW_CHECKSUM) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) $@

# firmware blob with main.mpy and checksum appended - can be flashed to hub
$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware file"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data -j .user -j .checksum $^ $@
	$(ECHO) "`wc -c < $@` bytes"

# firmware blob without main.mpy or checksum - use as base for appending other .mpy
$(BUILD)/firmware-base.bin: $(BUILD)/firmware-no-checksum.elf
	$(ECHO) "BIN creating firmware base file"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $@
	$(ECHO) "`wc -c < $@` bytes"

$(BUILD)/firmware.dfu: $(BUILD)/firmware.bin
	$(ECHO) "Create $@"
	$(Q)$(PYTHON) $(DFU) -b $(TEXT0_ADDR):$< $@

$(BUILD)/firmware.metadata.json: $(BUILD)/firmware-no-checksum.elf $(METADATA)
	$(ECHO) "META creating firmware metadata"
	$(Q)$(METADATA) $(FW_VERSION) $(PBIO_PLATFORM) $(MPY_CROSS_FLAGS) $<.map $@

$(BUILD)/firmware.zip: $(BUILD)/firmware-base.bin $(BUILD)/firmware.metadata.json main.py
	$(ECHO) "ZIP creating firmware package"
	$(Q)$(ZIP) -j $@ $^

deploy: $(BUILD)/firmware.dfu
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(PYDFU) -u $< $(if $(DFU_VID),--vid $(DFU_VID)) $(if $(DFU_PID),--pid $(DFU_PID))

deploy-openocd: $(BUILD)/firmware-no-checksum.bin
	$(ECHO) "Writing $< to the board via ST-LINK using OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "stm_flash $< $(TEXT0_ADDR)"

include $(TOP)/py/mkrules.mk
