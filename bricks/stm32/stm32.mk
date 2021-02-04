# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (c) 2019-2021 The Pybricks Authors

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
ifeq ($(PB_LIB_BTSTACK),1)
ifeq ("$(wildcard ../../lib/btstack/README.md)","")
$(info GIT cloning btstack submodule)
$(info $(shell cd ../.. && git submodule update --checkout --init lib/btstack))
ifeq ("$(wildcard ../../lib/btstack/README.md)","")
$(error failed)
endif
endif
endif

# lets micropython make files work with external files
USER_C_MODULES = $(PBTOP)

include ../../micropython/py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = ../pybricks_qstrdefs.h
QSTR_GLOBAL_DEPENDENCIES = $(PBTOP)/bricks/stm32/configport.h

FROZEN_MPY_TOOL_ARGS = -mlongint-impl=none

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1

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
ifeq ($(PB_LIB_BTSTACK),1)
INC += -I$(PBTOP)/lib/btstack/chipset/cc256x
INC += -I$(PBTOP)/lib/btstack/src
endif
ifeq ($(PB_USE_LSM6DS3TR_C),1)
INC += -I$(PBTOP)/lib/lsm6ds3tr_c_STdC/driver
endif
ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
INC += -I$(PBTOP)/lib/STM32_USB_Device_Library/Class/CDC/Inc/
INC += -I$(PBTOP)/lib/STM32_USB_Device_Library/Core/Inc/
endif
INC += -I$(PBTOP)
INC += -I$(BUILD)

GIT = git
ZIP = zip
DFU = $(TOP)/tools/dfu.py
PYDFU = $(TOP)/tools/pydfu.py
BUILD_DUAL_BOOT_BIN = $(PBTOP)/tools/build-dual-boot-bin.py
BUILD_DUAL_BOOT_INSTALLER = $(PBTOP)/tools/build-dual-boot-installer.py
CHECKSUM = $(PBTOP)/tools/checksum.py
CHECKSUM_TYPE ?= xor
METADATA = $(PBTOP)/tools/metadata.py
OPENOCD ?= openocd
OPENOCD_CONFIG ?= openocd_stm32$(PB_MCU_SERIES_LCASE).cfg
TEXT0_ADDR ?= 0x08000000

CFLAGS_MCU_F0 = -mthumb -mtune=cortex-m0 -mcpu=cortex-m0  -msoft-float
CFLAGS_MCU_F4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU_L4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS = $(INC) -Wall -Werror -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU_$(PB_MCU_SERIES)) $(COPT) $(CFLAGS_EXTRA)

# define external oscillator frequency
CFLAGS += -DHSE_VALUE=$(PB_MCU_EXT_OSC_HZ)

# linker scripts
LD_FILES = $(PBIO_PLATFORM).ld $(PBTOP)/bricks/stm32/common.ld

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


LIBS = "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

SRC_C = $(addprefix bricks/stm32/,\
	main.c \
	systick.c \
	uart_core.c \
	)

# Extra core MicroPython files

# NB: Since we are using MicroPython's build system, files in the micropython/
# directory have the micropython/ prefix excluded. It is very important to do
# it that way since there is special handling of certain files that will break
# if we don't do it this way. So we need to be very careful about name clashes
# between the top level directory and the micropython/ subdirectory.

SRC_C += $(addprefix lib/,\
	libc/string0.c \
	mp-readline/readline.c \
	utils/interrupt_char.c \
	utils/pyexec.c \
	utils/stdout_helpers.c \
	)

SRC_S = \
	lib/pbio/platform/$(PBIO_PLATFORM)/startup.s \

ifeq ($(PB_MCU_SERIES),F0)
	SRC_S += lib/utils/gchelper_m0.s
else
	SRC_S += lib/utils/gchelper_m0.s
endif

# Pybricks modules

PYBRICKS_PYBRICKS_SRC_C = $(addprefix pybricks/,\
	common/pb_type_battery.c \
	common/pb_type_colorlight_external.c \
	common/pb_type_colorlight_internal.c \
	common/pb_type_control.c \
	common/pb_type_dcmotor.c \
	common/pb_type_imu.c \
	common/pb_type_keypad.c \
	common/pb_type_lightarray.c \
	common/pb_type_lightmatrix_fonts.c \
	common/pb_type_lightmatrix.c \
	common/pb_type_logger.c \
	common/pb_type_motor.c \
	common/pb_type_speaker.c \
	experimental/pb_module_experimental.c \
	geometry/pb_module_geometry.c \
	geometry/pb_type_matrix.c \
	hubs/pb_module_hubs.c \
	hubs/pb_type_cityhub.c \
	hubs/pb_type_technichub.c \
	hubs/pb_type_movehub.c \
	hubs/pb_type_primehub.c \
	iodevices/pb_module_iodevices.c \
	iodevices/pb_type_iodevices_analogsensor.c \
	iodevices/pb_type_iodevices_ev3devsensor.c \
	iodevices/pb_type_iodevices_i2cdevice.c \
	iodevices/pb_type_iodevices_lumpdevice.c \
	iodevices/pb_type_iodevices_pupdevice.c \
	iodevices/pb_type_iodevices_uartdevice.c \
	media/pb_module_media.c \
	parameters/pb_type_icon.c \
	parameters/pb_module_parameters.c \
	parameters/pb_type_button.c \
	parameters/pb_type_color.c \
	parameters/pb_type_direction.c \
	parameters/pb_type_port.c \
	parameters/pb_type_side.c \
	parameters/pb_type_stop.c \
	pupdevices/pb_module_pupdevices.c \
	pupdevices/pb_type_pupdevices_colordistancesensor.c \
	pupdevices/pb_type_pupdevices_colorsensor.c \
	pupdevices/pb_type_pupdevices_forcesensor.c \
	pupdevices/pb_type_pupdevices_infraredsensor.c \
	pupdevices/pb_type_pupdevices_light.c \
	pupdevices/pb_type_pupdevices_pfmotor.c \
	pupdevices/pb_type_pupdevices_tiltsensor.c \
	pupdevices/pb_type_pupdevices_ultrasonicsensor.c \
	pybricks.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_drivebase.c \
	tools/pb_module_tools.c \
	tools/pb_type_stopwatch.c \
	util_mp/pb_obj_helper.c \
	util_mp/pb_type_enum.c \
	util_pb/pb_color_map.c \
	util_pb/pb_device_stm32.c \
	util_pb/pb_error.c \
	util_pb/pb_imu.c \
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
	gatt-service/nordic_spp_service_server.c \
	ancs_client.c \
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

HAL_SRC_C = $(addprefix lib/stm32lib/STM32$(PB_MCU_SERIES)xx_HAL_Driver/Src/,\
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_adc.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_cortex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dac_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dac.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_dma.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_gpio.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_i2c.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pcd_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pcd.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_pwr_ex.c \
	stm32$(PB_MCU_SERIES_LCASE)xx_hal_rcc_ex.c \
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
ifeq ($(PB_LIB_BTSTACK),1)
# use patched version of HAL UART from BTStack instead of MicroPython version
ifneq ($(PB_MCU_SERIES),F4)
$(error "BTStack is only supported on STM32F4xx")
endif
HAL_SRC_C := $(filter-out %xx_hal_uart.c, $(HAL_SRC_C))
HAL_SRC_C += lib/btstack/port/stm32-f4discovery-cc256x/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
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
	drv/$(PBIO_PLATFORM)/motor.c \
	drv/adc/adc_stm32_hal.c \
	drv/adc/adc_stm32f0.c \
	drv/battery/battery_adc.c \
	drv/bluetooth/bluetooth_btstack_control_gpio.c \
	drv/bluetooth/bluetooth_btstack_run_loop_contiki.c \
	drv/bluetooth/bluetooth_btstack_uart_block_stm32_hal.c \
	drv/bluetooth/bluetooth_btstack.c \
	drv/bluetooth/bluetooth_init_cc2564C_1.4.c \
	drv/button/button_adc.c \
	drv/button/button_gpio.c \
	drv/clock/clock_stm32.c \
	drv/core.c \
	drv/counter/counter_core.c \
	drv/counter/counter_stm32f0_gpio_quad_enc.c \
	drv/gpio/gpio_stm32f0.c \
	drv/gpio/gpio_stm32f4.c \
	drv/gpio/gpio_stm32l4.c \
	drv/ioport/ioport_lpf2.c \
	drv/led/led_array_pwm.c \
	drv/led/led_array.c \
	drv/led/led_core.c \
	drv/led/led_dual.c \
	drv/led/led_pwm.c \
	drv/pwm/pwm_core.c \
	drv/pwm/pwm_stm32_tim.c \
	drv/pwm/pwm_tlc5955_stm32.c \
	drv/reset/reset_stm32.c \
	drv/sound/sound_stm32_hal_dac.c \
	drv/uart/uart_stm32f0.c \
	drv/uart/uart_stm32f4_ll_irq.c \
	drv/uart/uart_stm32l4_ll_dma.c \
	drv/usb/stm32_usb_serial.c \
	platform/motors/settings.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	platform/$(PBIO_PLATFORM)/sys.c \
	src/color/conversion.c \
	src/control.c \
	src/dcmotor.c \
	src/drivebase.c \
	src/error.c \
	src/integrator.c \
	src/iodev.c \
	src/light/animation.c \
	src/light/color_light.c \
	src/light/light_matrix.c \
	src/logger.c \
	src/main.c \
	src/math.c \
	src/motor_process.c \
	src/observer.c \
	src/servo.c \
	src/tacho.c \
	src/trajectory_ext.c \
	src/trajectory.c \
	src/uartdev.c \
	sys/battery.c \
	sys/bluetooth.c \
	sys/hmi.c \
	sys/light_matrix.c \
	sys/light.c \
	sys/status.c \
	sys/supervisor.c \
	)

PBIO_DUAL_BOOT_SRC_C = $(addprefix lib/pbio/,\
	platform/$(PBIO_PLATFORM)/dual_boot.c \
	)

# STM32 IMU Library

LSM6DS3TR_C_SRC_C = lib/lsm6ds3tr_c_STdC/driver/lsm6ds3tr_c_reg.c

# MicroPython math library

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
OBJ += $(addprefix $(BUILD)/, $(PYBRICKS_PYBRICKS_SRC_C:.c=.o))
ifeq ($(PB_LIB_BLUENRG),1)
OBJ += $(addprefix $(BUILD)/, $(BLUENRG_SRC_C:.c=.o))
endif
ifeq ($(PB_LIB_BLE5STACK),1)
OBJ += $(addprefix $(BUILD)/, $(BLE5STACK_SRC_C:.c=.o))
endif
ifeq ($(PB_LIB_BTSTACK),1)
OBJ += $(addprefix $(BUILD)/, $(BTSTACK_SRC_C:.c=.o))
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
DUAL_BOOT_OBJ += $(addprefix $(BUILD)/, $(PBIO_DUAL_BOOT_SRC_C:.c=.o))
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
SRC_QSTR += $(SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

# Main firmware build targets
TARGETS := $(BUILD)/firmware.zip $(BUILD)/firmware.bin

# Optionally build project file for use with official LEGO app
ifeq ($(PB_DUAL_BOOT),1)
TARGETS += $(BUILD)/install_pybricks.llsp
endif

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

FW_CHECKSUM := $$($(CHECKSUM) $(CHECKSUM_TYPE) $(BUILD)/firmware-no-checksum.bin $(PB_FIRMWARE_MAX_SIZE))
FW_VERSION := $(shell $(GIT) describe --tags --dirty --always --exclude "@pybricks/*")

$(BUILD)/firmware-no-checksum.elf: $(LD_FILES) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) --defsym=CHECKSUM=0 $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) $@

# firmware blob used to calculate checksum
$(BUILD)/firmware-no-checksum.bin: $(BUILD)/firmware-no-checksum.elf
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data -j .user -j .checksum $^ $@

$(BUILD)/firmware.elf: $(BUILD)/firmware-no-checksum.bin $(OBJ)
	$(ECHO) "RELINK $@"
	$(Q)$(LD) --defsym=CHECKSUM=$(FW_CHECKSUM) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

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

# firmware blob with different starting flash memory address for dual booting
$(BUILD)/firmware-dual-boot-base.elf: $(LD_FILES) $(OBJ) $(DUAL_BOOT_OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(LD) --defsym=CHECKSUM=0 --defsym=DUAL_BOOT=1 $(LDFLAGS) -o $@ $(OBJ) $(DUAL_BOOT_OBJ) $(LIBS)
	$(Q)$(SIZE) $@

# firmware blob without main.mpy or checksum - use as base for appending other .mpy
$(BUILD)/firmware-dual-boot-base.bin: $(BUILD)/firmware-dual-boot-base.elf
	$(ECHO) "BIN creating dual-boot firmware base file"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data $^ $@
	$(ECHO) "`wc -c < $@` bytes"

# firmware blob without main.mpy or checksum - use as base for appending other .mpy
$(BUILD)/firmware-dual-boot.bin: $(BUILD)/firmware-dual-boot-base.bin $(BUILD_DUAL_BOOT_BIN) $(BASE_FW)
	$(Q)if [ -z "$(BASE_FW)" ]; then \
		echo "ERROR missing BASE_FW=... argument in make command"; \
		exit 1; \
	fi
	$(ECHO) "BIN creating dual-boot firmware file"
	$(Q)$(PYTHON) $(BUILD_DUAL_BOOT_BIN) $(BASE_FW) $< $@
	$(ECHO) "`wc -c < $@` bytes"

# firmware wrapped in special format for install with official apps
$(BUILD)/install_pybricks.llsp: $(BUILD)/firmware-dual-boot-base.bin
	$(ECHO) "Creating dual boot firmware installer"
	$(Q)$(PYTHON) $(BUILD_DUAL_BOOT_INSTALLER) $(FW_VERSION)

$(BUILD)/firmware.metadata.json: $(BUILD)/firmware-no-checksum.elf $(METADATA)
	$(ECHO) "META creating firmware metadata"
	$(Q)$(METADATA) $(FW_VERSION) $(PBIO_PLATFORM) $(MPY_CROSS_FLAGS) $<.map $@

# firmware.zip file
ZIP_FILES := \
	$(BUILD)/firmware-base.bin \
	$(BUILD)/firmware.metadata.json \
	main.py \
	ReadMe_OSS.txt \

ifeq ($(PB_DUAL_BOOT),1)
ZIP_FILES += $(BUILD)/firmware-dual-boot-base.bin
endif

$(BUILD)/firmware.zip: $(ZIP_FILES)
	$(ECHO) "ZIP creating firmware package"
	$(Q)$(ZIP) -j $@ $^

# firmware in DFU format
$(BUILD)/%.dfu: $(BUILD)/%.bin
	$(ECHO) "DFU Create $@"
	$(Q)$(PYTHON) $(DFU) -b $(TEXT0_ADDR):$< $@

deploy-dfu-%: $(BUILD)/%.dfu
	$(ECHO) "Writing $< to the board"
	$(Q)$(PYTHON) $(PYDFU) -u $< $(if $(DFU_VID),--vid $(DFU_VID)) $(if $(DFU_PID),--pid $(DFU_PID))

deploy-dfu: deploy-dfu-firmware

deploy-dfu-dual-boot: deploy-dfu-firmware-dual-boot

deploy-openocd: $(BUILD)/firmware-no-checksum.bin
	$(ECHO) "Writing $< to the board via ST-LINK using OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "stm_flash $< $(TEXT0_ADDR)"

include $(TOP)/py/mkrules.mk
