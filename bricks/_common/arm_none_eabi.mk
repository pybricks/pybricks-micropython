# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (c) 2019-2023 The Pybricks Authors

# This file is shared by all bare-metal Arm Pybricks ports.

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
PBTOP := ../$(patsubst %/_common/arm_none_eabi.mk,%,$(THIS_MAKEFILE))

# Bricks must specify the following variables in their Makefile

ifeq ($(PB_MCU_FAMILY),)
$(error "PB_MCU_FAMILY is not specified - add it in <hub>/Makefile)
endif

ifeq ($(PB_MCU_FAMILY),STM32)
ifeq ($(PB_MCU_SERIES),)
$(error "PB_MCU_SERIES is not specified - add it in <hub>/Makefile)
else
PB_MCU_SERIES_LCASE = $(subst F,f,$(subst L,l,$(PB_MCU_SERIES)))
endif
ifeq ($(PB_CMSIS_MCU),)
$(error "PB_CMSIS_MCU is not specified - add it in <hub>/Makefile")
endif
endif

ifeq ($(PBIO_PLATFORM),)
$(error "PBIO_PLATFORM is not specified - add it in <hub>/Makefile)
endif

# ensure required git submodules checked out
ifeq ("$(wildcard $(PBTOP)/micropython/README.md)","")
$(info GIT cloning micropython submodule)
$(info $(shell cd $(PBTOP) && git submodule update --init micropython))
ifeq ("$(wildcard $(PBTOP)/micropython/README.md)","")
$(error failed)
endif
endif
ifeq ("$(wildcard $(PBTOP)/micropython/lib/micropython-lib/README.md)","")
$(info GIT cloning micropython-lib submodule)
$(info $(shell cd $(PBTOP)/micropython && git submodule update --init lib/micropython-lib))
ifeq ("$(wildcard $(PBTOP)/micropython/lib/micropython-lib/README.md)","")
$(error failed)
endif
endif
ifeq ($(PB_LIB_STM32_HAL),1)
ifeq ("$(wildcard $(PBTOP)/micropython/lib/stm32lib/README.md)","")
$(info GIT cloning stm32lib submodule)
$(info $(shell cd $(PBTOP)/micropython && git submodule update --init lib/stm32lib))
ifeq ("$(wildcard $(PBTOP)/micropython/lib/stm32lib/README.md)","")
$(error failed)
endif
endif
endif
ifeq ($(PB_LIB_BTSTACK),1)
ifeq ("$(wildcard $(PBTOP)/lib/btstack/README.md)","")
$(info GIT cloning btstack submodule)
$(info $(shell cd $(PBTOP) && git submodule update --checkout --init lib/btstack))
ifeq ("$(wildcard $(PBTOP)/lib/btstack/README.md)","")
$(error failed)
endif
endif
endif
ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
ifeq ("$(wildcard $(PBTOP)/lib/STM32_USB_Device_Library/README.md)","")
$(info GIT cloning STM32_USB_Device_Library submodule)
$(info $(shell cd $(PBTOP) && git submodule update --checkout --init lib/STM32_USB_Device_Library))
ifeq ("$(wildcard $(PBTOP)/lib/STM32_USB_Device_Library/README.md)","")
$(error failed)
endif
endif
endif

# lets micropython make files work with external files
USER_C_MODULES = $(PBTOP)

include $(PBTOP)/micropython/py/mkenv.mk

# Include common frozen modules.
ifeq ($(PB_FROZEN_MODULES),1)
ifneq ("$(wildcard $(PBTOP)/bricks/_common/modules/*.py)","")
FROZEN_MANIFEST ?= ../_common/manifest.py
endif
endif

# qstr definitions (must come before including py.mk)
QSTR_DEFS = $(PBTOP)/bricks/_common/qstrdefs.h
QSTR_GLOBAL_DEPENDENCIES = $(PBTOP)/bricks/_common/mpconfigport.h
ifeq ($(PB_MCU_FAMILY),STM32)
QSTR_GLOBAL_DEPENDENCIES += ../_common_stm32/mpconfigport.h
endif

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

CROSS_COMPILE ?= arm-none-eabi-

INC += -I.
INC += -I$(TOP)
ifeq ($(PB_MCU_FAMILY),STM32)
INC += -I$(TOP)/lib/cmsis/inc
INC += -I$(TOP)/lib/stm32lib/CMSIS/STM32$(PB_MCU_SERIES)xx/Include
endif
ifeq ($(PB_LIB_STM32_HAL),1)
INC += -I$(TOP)/lib/stm32lib/STM32$(PB_MCU_SERIES)xx_HAL_Driver/Inc
endif
INC += -I$(PBTOP)/lib/contiki-core
INC += -I$(PBTOP)/lib/lego
INC += -I$(PBTOP)/lib/lwrb/src/include
INC += -I$(PBTOP)/lib/pbio/include
INC += -I$(PBTOP)/lib/pbio/platform/$(PBIO_PLATFORM)
INC += -I$(PBTOP)/lib/pbio
ifeq ($(PB_LIB_BLUENRG),1)
INC += -I$(PBTOP)/lib/BlueNRG-MS/includes
endif
ifeq ($(PB_LIB_BLE5STACK),1)
INC += -I$(PBTOP)/lib/ble5stack/central
endif
ifeq ($(PB_LIB_BTSTACK),1)
INC += -I$(PBTOP)/lib/btstack/chipset/cc256x
INC += -I$(PBTOP)/lib/btstack/src
endif
ifeq ($(PB_LIB_LSM6DS3TR_C),1)
INC += -I$(PBTOP)/lib/lsm6ds3tr_c_STdC/driver
endif
ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
INC += -I$(PBTOP)/lib/STM32_USB_Device_Library/Core/Inc/
endif
INC += -I$(PBTOP)
INC += -I$(BUILD)

GIT = git
ZIP = zip
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
PYBRICKSDEV = pybricksdev
METADATA = $(PBTOP)/tools/metadata.py
OPENOCD ?= openocd
OPENOCD_CONFIG ?= openocd_stm32$(PB_MCU_SERIES_LCASE).cfg
TEXT0_ADDR ?= 0x08000000

ifeq ($(PB_MCU_FAMILY),STM32)
CFLAGS_MCU_F0 = -mthumb -mtune=cortex-m0 -mcpu=cortex-m0 -msoft-float
CFLAGS_MCU_F4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU_L4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU = $(CFLAGS_MCU_$(PB_MCU_SERIES))
else
ifeq ($(PB_MCU_FAMILY),AT91SAM7)
CFLAGS_MCU = -mthumb -mthumb-interwork -mtune=arm7tdmi -mcpu=arm7tdmi -msoft-float
else
$(error unsupported PB_MCU_FAMILY)
endif
endif

CFLAGS_WARN = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-maybe-uninitialized
CFLAGS = $(INC) -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU) $(CFLAGS_WARN) $(COPT) $(CFLAGS_EXTRA)
$(BUILD)/lib/libm/%.o: CFLAGS += -Wno-sign-compare

# linker scripts
LD_FILES = $(PBTOP)/lib/pbio/platform/$(PBIO_PLATFORM)/platform.ld
ifeq ($(PB_MCU_FAMILY),STM32)
LD_FILES += $(PBTOP)/lib/pbio/platform/arm_common.ld
endif

LDFLAGS = $(addprefix -T,$(LD_FILES)) -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections

SUPPORTS_HARDWARE_FP_SINGLE = 0
ifeq ($(PB_MCU_FAMILY),STM32)
ifeq ($(PB_MCU_SERIES),$(filter $(PB_MCU_SERIES),F4 L4))
SUPPORTS_HARDWARE_FP_SINGLE = 1
endif
endif

# avoid doubles
CFLAGS += -fsingle-precision-constant -Wdouble-promotion

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -Og -ggdb
else
CFLAGS += -Os -DNDEBUG -flto
CFLAGS += -fdata-sections -ffunction-sections
endif

ifeq ($(PB_MCU_FAMILY),STM32)
# Required for STM32 library
CFLAGS += -D$(PB_CMSIS_MCU)
# Required by pbio drivers
CFLAGS += -DSTM32_H='<stm32$(PB_MCU_SERIES_LCASE)xx.h>'
CFLAGS += -DSTM32_HAL_H='<stm32$(PB_MCU_SERIES_LCASE)xx_hal.h>'
endif

LIBS = "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

# Sources and libraries common to all pybricks bricks

include $(PBTOP)/bricks/_common/sources.mk

# Extra core MicroPython files

# NB: Since we are using MicroPython's build system, files in the micropython/
# directory have the micropython/ prefix excluded. It is very important to do
# it that way since there is special handling of certain files that will break
# if we don't do it this way. So we need to be very careful about name clashes
# between the top level directory and the micropython/ subdirectory.

PY_EXTRA_SRC_C = $(addprefix shared/,\
	libc/string0.c \
	runtime/gchelper_native.c \
	runtime/interrupt_char.c \
	runtime/pyexec.c \
	runtime/stdout_helpers.c \
	runtime/sys_stdio_mphal.c \
	)

ifneq ($(PBIO_PLATFORM),move_hub)
# to avoid adding unused root pointers
PY_EXTRA_SRC_C += shared/readline/readline.c
endif

PY_EXTRA_SRC_C += $(addprefix bricks/_common/,\
	micropython.c \
	)

ifeq ($(PB_MCU_FAMILY),STM32)
PY_EXTRA_SRC_C += $(addprefix bricks/_common_stm32/,\
	mphalport.c \
	)

ifeq ($(PB_MCU_SERIES),F0)
SRC_S += shared/runtime/gchelper_thumb1.s
else
SRC_S += shared/runtime/gchelper_thumb2.s
endif
endif

ifeq ($(PB_MCU_FAMILY),AT91SAM7)
PY_EXTRA_SRC_C += $(addprefix bricks/nxt/,\
	mphalport.c \
	)

SRC_S += shared/runtime/gchelper_thumb1.s
endif

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

# BlueKitchen Bluetooth stack

BTSTACK_SRC_C = $(addprefix lib/btstack/src/,\
	ad_parser.c \
	btstack_audio.c \
	btstack_base64_decoder.c \
	btstack_crypto.c \
	btstack_hid_parser.c \
	btstack_linked_list.c \
	btstack_memory.c \
	btstack_memory_pool.c \
	btstack_ring_buffer.c \
	btstack_run_loop.c \
	btstack_slip.c \
	btstack_tlv.c \
	btstack_util.c \
	hci.c \
	hci_cmd.c \
	hci_dump.c \
	hci_transport_em9304_spi.c \
	hci_transport_h4.c \
	hci_transport_h5.c \
	l2cap.c \
	l2cap_signaling.c \
	)

BTSTACK_SRC_C += $(addprefix lib/btstack/src/ble/,\
	gatt-service/device_information_service_server.c \
	gatt-service/nordic_spp_service_server.c \
	att_db_util.c \
	att_db.c \
	att_dispatch.c \
	att_server.c \
	gatt_client.c \
	le_device_db_memory.c \
	sm.c \
	)

BTSTACK_SRC_C += $(addprefix lib/btstack/chipset/cc256x/,\
	btstack_chipset_cc256x.c \
	)

# STM32 HAL

COPT += -DUSE_FULL_LL_DRIVER

STM32_HAL_SRC_C = $(addprefix lib/stm32lib/STM32$(PB_MCU_SERIES)xx_HAL_Driver/Src/,\
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_cortex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dac_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dac.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dma.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_flash.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_flash_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_fmpi2c.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_gpio.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_i2c.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pcd_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pcd.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pwr_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_rcc_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_rcc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_rng.c \
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

# some HAL drivers are not available on all MCUs
ifeq ($(PB_MCU_SERIES),F0)
STM32_HAL_SRC_C := $(filter-out %xx_hal_rng.c, $(STM32_HAL_SRC_C))
endif
ifeq ($(PB_MCU_SERIES),F4)
STM32_HAL_SRC_C := $(filter-out %xx_hal_uart_ex.c, $(STM32_HAL_SRC_C))
endif
ifneq ($(PB_MCU_SERIES),F4)
STM32_HAL_SRC_C := $(filter-out %xx_hal_fmpi2c.c, $(STM32_HAL_SRC_C))
endif
ifneq ($(PB_MCU_SERIES),L4)
STM32_HAL_SRC_C := $(filter-out %xx_ll_lpuart.c, $(STM32_HAL_SRC_C))
endif
ifneq ($(PB_LIB_STM32_USB_DEVICE),1)
STM32_HAL_SRC_C := $(filter-out %xx_hal_pcd_ex.c, $(STM32_HAL_SRC_C))
STM32_HAL_SRC_C := $(filter-out %xx_hal_pcd.c, $(STM32_HAL_SRC_C))
STM32_HAL_SRC_C := $(filter-out %xx_ll_usb.c, $(STM32_HAL_SRC_C))
endif

# STM32 IMU Library

LSM6DS3TR_C_SRC_C = lib/lsm6ds3tr_c_STdC/driver/lsm6ds3tr_c_reg.c

# STM32 USB Device library

SRC_STM32_USB_DEV = $(addprefix lib/STM32_USB_Device_Library/,\
	Core/Src/usbd_core.c \
	Core/Src/usbd_ctlreq.c \
	Core/Src/usbd_ioreq.c \
	)

ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
	INC += -I$(PBTOP)/lib/pbio/drv/usb/stm32_usbd
endif

SRC_STM32_USB_DEV += $(addprefix lib/pbio/drv/usb/stm32_usbd/,\
	usbd_conf.c \
	usbd_desc.c \
	usbd_pybricks.c \
	)

NXOS_SRC_C = $(addprefix lib/pbio/platform/nxt/nxos/,\
	_abort.c \
	assert.c \
	lock.c \
	util.c \
	display.c \
	interrupts.c \
	drivers/_efc.c \
	drivers/_lcd.c \
	drivers/_twi.c \
	drivers/_uart.c \
	drivers/aic.c \
	drivers/avr.c \
	drivers/bt.c \
	drivers/i2c_memory.c \
	drivers/i2c.c \
	drivers/motors.c \
	drivers/radar.c \
	drivers/rs485.c \
	drivers/sensors.c \
	drivers/usb.c \
	)

NXOS_SRC_S = $(addprefix lib/pbio/platform/nxt/nxos/,\
	irq.s \
	)

SRC_S += lib/pbio/platform/$(PBIO_PLATFORM)/startup.s

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PY_EXTRA_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))

OBJ += $(addprefix $(BUILD)/, $(CONTIKI_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LWRB_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))

ifeq ($(PB_LIB_BLUENRG),1)
OBJ += $(addprefix $(BUILD)/, $(BLUENRG_SRC_C:.c=.o))
endif

ifeq ($(PB_LIB_BLE5STACK),1)
OBJ += $(addprefix $(BUILD)/, $(BLE5STACK_SRC_C:.c=.o))
endif

ifeq ($(PB_LIB_BTSTACK),1)
OBJ += $(addprefix $(BUILD)/, $(BTSTACK_SRC_C:.c=.o))
endif

ifeq ($(PB_LIB_STM32_HAL),1)
OBJ += $(addprefix $(BUILD)/, $(STM32_HAL_SRC_C:.c=.o))
$(BUILD)/lib/stm32lib/%.o: CFLAGS += -Wno-sign-compare
# define external oscillator frequency
ifeq ($(PB_MCU_EXT_OSC_HZ),)
$(error "PB_MCU_EXT_OSC_HZ is not specified - add it in <hub>/Makefile)
else
CFLAGS += -DHSE_VALUE=$(PB_MCU_EXT_OSC_HZ)
endif
endif

ifeq ($(PB_LIB_LSM6DS3TR_C),1)
OBJ += $(addprefix $(BUILD)/, $(LSM6DS3TR_C_SRC_C:.c=.o))
endif

ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_STM32_USB_DEV:.c=.o))
$(BUILD)/lib/STM32_USB_Device_Library/%.o: CFLAGS += -Wno-sign-compare
endif

ifeq ($(PBIO_PLATFORM),nxt)
OBJ += $(addprefix $(BUILD)/, $(NXOS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(NXOS_SRC_S:.s=.o))
endif

# List of sources for qstr extraction
SRC_QSTR += $(PY_EXTRA_SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

ifneq ($(FROZEN_MANIFEST),)
CFLAGS += -DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
CFLAGS += -DMICROPY_MODULE_FROZEN_MPY
MPY_TOOL_FLAGS += -mlongint-impl none
endif

# Main firmware build targets
TARGETS := $(BUILD)/firmware.zip

all: $(TARGETS)

# handle BTStack .gatt files

ifeq ($(PB_LIB_BTSTACK),1)

GATT_FILES := $(addprefix lib/pbio/drv/bluetooth/,\
	pybricks_service.gatt \
	)

GATT_H_FILES := $(addprefix $(BUILD)/genhdr/, $(notdir $(GATT_FILES:.gatt=.h)))

$(BUILD)/lib/pbio/drv/bluetooth/bluetooth_btstack.o: $(GATT_H_FILES)

$(BUILD)/genhdr/%.h: $(PBTOP)/lib/pbio/drv/bluetooth/%.gatt
	$(Q)$(PYTHON) $(PBTOP)/lib/btstack/tool/compile_gatt.py $< $@

endif

ifeq ($(MICROPY_GIT_TAG),)
FW_VERSION := $(shell $(GIT) describe --tags --dirty --always --exclude "@pybricks/*")
else
# CI builds use build number + git hash as tag/firmware version
FW_VERSION := $(MICROPY_GIT_TAG)
endif

ifeq ($(PB_MCU_FAMILY),STM32)
FW_SECTIONS := -j .isr_vector -j .text -j .data -j .name
else
FW_SECTIONS :=
endif

$(BUILD)/firmware.elf: $(LD_FILES) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) -A $@

# firmware blob without main.mpy or checksum - use as base for appending other .mpy
$(BUILD)/firmware-base.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware base file"
	$(Q)$(OBJCOPY) -O binary $(FW_SECTIONS) $^ $@
	$(ECHO) "`wc -c < $@` bytes"

$(BUILD)/firmware.metadata.json: $(BUILD)/firmware.elf $(METADATA)
	$(ECHO) "META creating firmware metadata"
	$(Q)$(METADATA) $(FW_VERSION) $(PBIO_PLATFORM) $<.map $@

# firmware.zip file
ZIP_FILES := \
	$(BUILD)/firmware-base.bin \
	$(BUILD)/firmware.metadata.json \
	ReadMe_OSS.txt \

$(BUILD)/firmware.zip: $(ZIP_FILES)
	$(ECHO) "ZIP creating firmware package"
	$(Q)$(ZIP) -j $@ $^

# firmware in DFU format
$(BUILD)/%.dfu: $(BUILD)/%-base.bin
	$(ECHO) "DFU Create $@"
	$(Q)$(PYTHON) $(DFU) -b $(TEXT0_ADDR):$< $@

deploy: $(BUILD)/firmware.zip
	$(Q)$(PYBRICKSDEV) flash $< $(if $(filter $(PBIO_PLATFORM),nxt),,--name $(PBIO_PLATFORM))

deploy-openocd: $(BUILD)/firmware-base.bin
	$(ECHO) "Writing $< to the board via ST-LINK using OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "stm_flash $< $(TEXT0_ADDR)"

include $(TOP)/py/mkrules.mk
