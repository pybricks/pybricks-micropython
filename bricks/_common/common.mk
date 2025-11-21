# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (c) 2019-2023 The Pybricks Authors

# This file is shared by all bare-metal Arm Pybricks ports.

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
PBTOP := ../$(patsubst %/_common/common.mk,%,$(THIS_MAKEFILE))

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
ifneq ($(strip $(PB_LIB_BTSTACK)),)
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
ifeq ($(PB_LIB_UMM_MALLOC),1)
ifeq ("$(wildcard $(PBTOP)/lib/umm_malloc/README.md)","")
$(info GIT cloning umm_malloc submodule)
$(info $(shell cd $(PBTOP) && git submodule update --checkout --init lib/umm_malloc))
ifeq ("$(wildcard $(PBTOP)/lib/umm_malloc/README.md)","")
$(error failed)
endif
endif
endif

# lets micropython make files work with external files
USER_C_MODULES = $(PBTOP)

include $(PBTOP)/micropython/py/mkenv.mk

# Include common frozen modules.
ifeq ($(PB_FROZEN_MODULES),1)
FROZEN_MANIFEST ?= manifest.py
endif

# qstr definitions (must come before including py.mk)
QSTR_DEFS = $(PBTOP)/bricks/_common/qstrdefs.h
QSTR_GLOBAL_DEPENDENCIES = $(PBTOP)/bricks/_common/mpconfigport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

INC += -I.
INC += -I$(TOP)
ifeq ($(PB_MCU_FAMILY),STM32)
INC += -I$(TOP)/lib/cmsis/inc
INC += -I$(TOP)/lib/stm32lib/CMSIS/STM32$(PB_MCU_SERIES)xx/Include
endif
ifeq ($(PB_LIB_STM32_HAL),1)
INC += -I$(TOP)/lib/stm32lib/STM32$(PB_MCU_SERIES)xx_HAL_Driver/Inc
endif
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
ifneq ($(strip $(PB_LIB_BTSTACK)),)
INC += -I$(PBTOP)/lib/btstack/chipset/cc256x
INC += -I$(PBTOP)/lib/btstack/src
endif
ifeq ($(PB_LIB_LSM6DS3TR_C),1)
INC += -I$(PBTOP)/lib/lsm6ds3tr_c_STdC/driver
endif
ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
INC += -I$(PBTOP)/lib/STM32_USB_Device_Library/Core/Inc/
endif
ifeq ($(PB_MCU_FAMILY),TIAM1808)
INC += -I$(PBTOP)/lib/pbio/platform/ev3/osek
INC += -I$(PBTOP)/lib/tiam1808
INC += -I$(PBTOP)/lib/tiam1808/tiam1808
INC += -I$(PBTOP)/lib/tiam1808/tiam1808/hw
INC += -I$(PBTOP)/lib/tiam1808/tiam1808/armv5
INC += -I$(PBTOP)/lib/tiam1808/tiam1808/armv5/am1808
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

ifeq ($(PB_MCU_FAMILY),native)
UNAME_S := $(shell uname -s)
LD = $(CC)
CFLAGS += $(INC) -Wall -Werror -Wdouble-promotion -Wfloat-conversion -std=gnu99 $(COPT) -D_GNU_SOURCE
ifeq ($(UNAME_S),Linux)
LDFLAGS += -Wl,-Map=$@.map,--cref -Wl,--gc-sections
else ifeq ($(UNAME_S),Darwin)
LDFLAGS += -Wl,-map,$@.map -Wl,-dead_strip
endif
LIBS = -lm
else # end native, begin embedded
CROSS_COMPILE ?= arm-none-eabi-
ifeq ($(PB_MCU_FAMILY),STM32)
CFLAGS_MCU_F0 = -mthumb -mtune=cortex-m0 -mcpu=cortex-m0 -msoft-float
CFLAGS_MCU_F4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU_L4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU = $(CFLAGS_MCU_$(PB_MCU_SERIES))
else
ifeq ($(PB_MCU_FAMILY),AT91SAM7)
CFLAGS_MCU = -mthumb -mthumb-interwork -mtune=arm7tdmi -mcpu=arm7tdmi -msoft-float
else
ifeq ($(PB_MCU_FAMILY),TIAM1808)
CFLAGS_MCU = -mcpu=arm926ej-s -Dgcc -Dam1808
else
$(error unsupported PB_MCU_FAMILY)
endif
endif
endif

CFLAGS_WARN = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-maybe-uninitialized
CFLAGS = $(INC) -std=c11 -nostdlib -fshort-enums $(CFLAGS_MCU) $(CFLAGS_WARN) $(COPT) $(CFLAGS_EXTRA)
$(BUILD)/lib/libm/%.o: CFLAGS += -Wno-sign-compare

# linker scripts
LD_FILES = $(PBTOP)/lib/pbio/platform/$(PBIO_PLATFORM)/platform.ld
ifeq ($(PB_MCU_FAMILY),STM32)
LD_FILES += $(PBTOP)/lib/pbio/platform/arm_common.ld
endif

LDFLAGS = $(addprefix -T,$(LD_FILES)) -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections
ifeq ($(PB_MCU_FAMILY),TIAM1808)
# "nmagic" mode
# This option (with a legacy name) is used to disable page alignment of sections,
# which makes the resulting ELF file smaller as padding will be eliminated.
LDFLAGS += -n
endif

SUPPORTS_HARDWARE_FP_SINGLE = 0
ifeq ($(PB_MCU_FAMILY),STM32)
ifeq ($(PB_MCU_SERIES),$(filter $(PB_MCU_SERIES),F4 L4))
SUPPORTS_HARDWARE_FP_SINGLE = 1
endif
endif

# avoid doubles
CFLAGS += -fsingle-precision-constant -Wdouble-promotion

endif # end embedded, begin common

# Tune for Debugging or Optimization
ifeq ($(COVERAGE), 1)
CFLAGS += --coverage -fprofile-arcs -ftest-coverage
LDFLAGS += --coverage
else ifeq ($(DEBUG), 1)
CFLAGS += -O0 -ggdb
else ifeq ($(DEBUG), 2)
CFLAGS += -Os -DNDEBUG -flto=auto
else
CFLAGS += -Os -DNDEBUG -flto=auto
endif

CFLAGS += -fdata-sections -ffunction-sections

ifeq ($(PB_MCU_FAMILY),STM32)
# Required for STM32 library
CFLAGS += -D$(PB_CMSIS_MCU)
# Required by pbio drivers
CFLAGS += -DSTM32_H='<stm32$(PB_MCU_SERIES_LCASE)xx.h>'
CFLAGS += -DSTM32_HAL_H='<stm32$(PB_MCU_SERIES_LCASE)xx_hal.h>'
endif

LIBS += "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

# Sources and libraries common to all pybricks bricks

include $(PBTOP)/bricks/_common/sources.mk

# Extra core MicroPython files

# NB: Since we are using MicroPython's build system, files in the micropython/
# directory have the micropython/ prefix excluded. It is very important to do
# it that way since there is special handling of certain files that will break
# if we don't do it this way. So we need to be very careful about name clashes
# between the top level directory and the micropython/ subdirectory.

PY_EXTRA_SRC_C = $(addprefix shared/,\
	runtime/interrupt_char.c \
	runtime/pyexec.c \
	runtime/stdout_helpers.c \
	)

ifeq ($(PB_MCU_FAMILY),native)
PY_EXTRA_SRC_C += $(addprefix shared/,\
	runtime/gchelper_generic.c \
	runtime/sys_stdio_mphal.c \
	)
PY_EXTRA_SRC_C += $(addprefix bricks/virtualhub/,\
	pbio_os_hook.c \
	)
else
PY_EXTRA_SRC_C += $(addprefix shared/,\
	libc/string0.c \
	runtime/gchelper_native.c \
	runtime/sys_stdio_mphal.c \
	)
endif

ifneq ($(PBIO_PLATFORM),move_hub)
# to avoid adding unused root pointers
PY_EXTRA_SRC_C += shared/readline/readline.c
endif

PY_EXTRA_SRC_C += $(addprefix bricks/_common/,\
	micropython.c \
	mphalport.c \
	)

# Not all MCUs support thumb2 instructions.
ifeq ($(PB_MCU_FAMILY),native)
SRC_S +=
else ifeq ($(PB_MCU_SERIES),$(filter $(PB_MCU_SERIES),AT91SAM7 F0 TIAM1808))
SRC_S += shared/runtime/gchelper_thumb1.s
else
SRC_S += shared/runtime/gchelper_thumb2.s
endif

# Skipping uart_irda_cir.c, gpio_v2.c, and hsi2c.c usbphyGS70.c, which
# partially overlap with uart.c, gpio.c, and i2c.c, usbphyGS70.c
TI_AM1808_SRC_C = $(addprefix lib/tiam1808/,\
	drivers/cppi41dma.c \
	drivers/cpsw.c \
	drivers/dmtimer.c \
	drivers/ecap.c \
	drivers/edma.c \
	drivers/ehrpwm.c \
	drivers/emifa.c \
	drivers/gpio.c \
	drivers/gpmc.c \
	drivers/hs_mmcsd.c \
	drivers/i2c.c \
	drivers/mcasp.c \
	drivers/mcspi.c \
	drivers/mdio.c \
	drivers/pruss.c \
	drivers/psc.c \
	drivers/rtc.c \
	drivers/spi.c \
	drivers/syscfg.c \
	drivers/timer.c \
	drivers/uart.c \
	drivers/usb.c \
	drivers/usbphyGS60.c \
	drivers/watchdog.c \
	system_config/armv5/am1808/interrupt.c \
	system_config/armv5/gcc/cp15.c \
	system_config/armv5/gcc/cpu.c \
	)

TI_AM1808_SRC_C += $(addprefix lib/pbio/drv/uart/uart_ev3_pru_lib/,\
	pru.c \
	suart_api.c \
	suart_utils.c \
	)

EV3_SRC_S = $(addprefix lib/pbio/platform/ev3/,\
	exceptionhandler.S \
	start.S \
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

# BlueKitchen Bluetooth stack

BTSTACK_SRC_C = $(addprefix lib/btstack/src/,\
	ad_parser.c \
	btstack_audio.c \
	btstack_base64_decoder.c \
	btstack_crypto.c \
	btstack_hid_parser.c \
	btstack_linked_list.c \
	btstack_memory_pool.c \
	btstack_memory.c \
	btstack_ring_buffer.c \
	btstack_run_loop.c \
	btstack_slip.c \
	btstack_tlv.c \
	btstack_util.c \
	hci_cmd.c \
	hci_dump.c \
	hci_transport_em9304_spi.c \
	hci_transport_h4.c \
	hci_transport_h5.c \
	hci.c \
	l2cap_signaling.c \
	l2cap.c \
	)

BTSTACK_BLE_SRC_C += $(addprefix lib/btstack/src/ble/,\
	att_db_util.c \
	att_db.c \
	att_dispatch.c \
	att_server.c \
	gatt_client.c \
	gatt-service/device_information_service_server.c \
	gatt-service/nordic_spp_service_server.c \
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
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_flash_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_flash.c \
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

# umm_malloc library

SRC_UMM_MALLOC = lib/umm_malloc/src/umm_malloc.c

ifeq ($(PB_LIB_UMM_MALLOC),1)
CFLAGS += -I$(PBTOP)/lib/umm_malloc/src
endif

# NXT OS

NXOS_SRC_C = $(addprefix lib/pbio/platform/nxt/nxos/,\
	_abort.c \
	assert.c \
	display.c \
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
	interrupts.c \
	lock.c \
	util.c \
	)

NXOS_SRC_S = $(addprefix lib/pbio/platform/nxt/nxos/,\
	irq.s \
	)

ifneq ($(PB_MCU_FAMILY),TIAM1808)
SRC_S += lib/pbio/platform/$(PBIO_PLATFORM)/startup.s
endif

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(PY_EXTRA_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))

OBJ += $(addprefix $(BUILD)/, $(LWRB_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LEGO_SPEC_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))

ifeq ($(PB_LIB_BLUENRG),1)
OBJ += $(addprefix $(BUILD)/, $(BLUENRG_SRC_C:.c=.o))
endif

ifeq ($(PB_LIB_BLE5STACK),1)
OBJ += $(addprefix $(BUILD)/, $(BLE5STACK_SRC_C:.c=.o))
endif

ifeq ($(PB_LIB_BTSTACK),classic)
OBJ += $(addprefix $(BUILD)/, $(BTSTACK_SRC_C:.c=.o))
endif

ifeq ($(PB_LIB_BTSTACK),lowenergy)
OBJ += $(addprefix $(BUILD)/, $(BTSTACK_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(BTSTACK_BLE_SRC_C:.c=.o))
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

ifeq ($(PB_LIB_UMM_MALLOC),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_UMM_MALLOC:.c=.o))
endif

ifeq ($(PBIO_PLATFORM),nxt)
OBJ += $(addprefix $(BUILD)/, $(NXOS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(NXOS_SRC_S:.s=.o))
endif

ifeq ($(PB_MCU_FAMILY),TIAM1808)
OBJ += $(addprefix $(BUILD)/, $(TI_AM1808_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(EV3_SRC_S:.S=.o))
$(addprefix $(BUILD)/, $(EV3_SRC_S:.S=.o)): CFLAGS += -D__ASSEMBLY__
OBJ += $(BUILD)/pru_suart.bin.o
OBJ += $(BUILD)/pru_ledpwm.bin.o
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

ifneq ($(PB_MCU_FAMILY),native)
# Main firmware build targets
TARGETS := $(BUILD)/firmware.zip
else
TARGETS := $(BUILD)/firmware.elf
endif

all: $(TARGETS)

# handle BTStack .gatt files (only for BLE / lowenergy)

ifeq ($(PB_LIB_BTSTACK),lowenergy)

GATT_FILES := $(addprefix lib/pbio/drv/bluetooth/,\
	pybricks_service.gatt \
	)

GATT_H_FILES := $(addprefix $(BUILD)/genhdr/, $(notdir $(GATT_FILES:.gatt=.h)))

$(BUILD)/lib/pbio/drv/bluetooth/bluetooth_btstack.o: $(GATT_H_FILES)

$(BUILD)/genhdr/%.h: $(PBTOP)/lib/pbio/drv/bluetooth/%.gatt
	$(Q)$(PYTHON) $(PBTOP)/lib/btstack/tool/compile_gatt.py $< $@

endif

ifeq ($(MICROPY_GIT_TAG),)
# CI builds use build number + git hash as tag/firmware version
export MICROPY_GIT_TAG := local-build-$(shell $(GIT) describe --tags --dirty --always --exclude "@pybricks/*")
export MICROPY_GIT_HASH :=$(shell $(GIT) rev-parse --short HEAD)
endif
FW_VERSION := $(MICROPY_GIT_TAG)

$(info PLATFORM: $(PBIO_PLATFORM) VERSION: $(FW_VERSION))

ifeq ($(PB_MCU_FAMILY),STM32)
FW_SECTIONS := -j .isr_vector -j .text -j .data -j .name
else
FW_SECTIONS :=
endif

$(BUILD)/firmware.elf: $(LD_FILES) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) -A $@

$(BUILD)/firmware.stripped.elf: $(BUILD)/firmware.elf
	$(ECHO) "STRIP $@"
	$(Q)$(STRIP) $< -o $@

# firmware blob without checksum
$(BUILD)/firmware-obj.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware base file"
	$(Q)$(OBJCOPY) -O binary $(FW_SECTIONS) $^ $@
	$(ECHO) "`wc -c < $@` bytes"

ifeq ($(PB_MCU_FAMILY),TIAM1808)

# REVISIT: downloading things doesn't belong in a Makefile.
$(BUILD)/u-boot.bin:
	$(ECHO) "Downloading u-boot.bin"
	$(Q)mkdir -p $(dir $@)
	$(Q)curl -sL -o $@ https://github.com/pybricks/u-boot/releases/download/pybricks/v2.0.1/u-boot.bin
	$(Q)echo "86ddad84f64d8aea85b4315fc1414bdec0bb0d46c92dbd3db45ed599e3a994cb  $@" | sha256sum -c --strict
$(BUILD)/pru_ledpwm.bin:
	$(ECHO) "Downloading pru_ledpwm.bin"
	$(Q)mkdir -p $(dir $@)
	$(Q)curl -sL -o $@ https://github.com/pybricks/pybricks-pru/releases/download/v1.0.0/pru_ledpwm.bin
	$(Q)echo "b4f1225e277bb22efa5394ce782cc19a3e2fdd54367e40b9d09e9ca99c6ef6d0  $@" | sha256sum -c --strict

MAKE_BOOTABLE_IMAGE = $(PBTOP)/bricks/ev3/make_bootable_image.py

# For EV3, merge firmware blob with u-boot to create a bootable image.
$(BUILD)/firmware-base.bin: $(MAKE_BOOTABLE_IMAGE) $(BUILD)/u-boot.bin $(BUILD)/firmware.stripped.elf
	$(Q)$^ $@

else
# For embeded systems, the firmware is just the base file.
$(BUILD)/firmware-base.bin: $(BUILD)/firmware-obj.bin
	$(Q)cp $< $@
endif

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

# PRU firmware
$(BUILD)/pru_suart.bin.o: $(PBTOP)/lib/pbio/drv/uart/uart_ev3_pru_lib/pru_suart.bin
	$(Q)$(OBJCOPY) -I binary -O elf32-littlearm -B arm \
		--rename-section .data=.pru0,alloc,load,readonly,data,contents $^ $@
$(BUILD)/pru_ledpwm.bin.o: $(BUILD)/pru_ledpwm.bin
	$(Q)$(OBJCOPY) -I binary -O elf32-littlearm -B arm \
		--rename-section .data=.pru1,alloc,load,readonly,data,contents $^ $@

# firmware in DFU format
$(BUILD)/%.dfu: $(BUILD)/%-base.bin
	$(ECHO) "DFU Create $@"
	$(Q)$(PYTHON) $(DFU) -b $(TEXT0_ADDR):$< $@

deploy: $(BUILD)/firmware.zip
	$(Q)$(PYBRICKSDEV) flash $< $(if $(filter-out nxt ev3,$(PBIO_PLATFORM)),--name $(PBIO_PLATFORM))

deploy-openocd: $(BUILD)/firmware-base.bin
	$(ECHO) "Writing $< to the board via ST-LINK using OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "stm_flash $< $(TEXT0_ADDR)"

# Run emulation build on a POSIX system using normal stdio
run: $(BUILD)/firmware.elf
	@$(BUILD)/firmware.elf

include $(TOP)/py/mkrules.mk
