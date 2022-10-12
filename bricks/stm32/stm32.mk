# SPDX-License-Identifier: MIT
# Copyright (c) 2013, 2014 Damien P. George
# Copyright (c) 2019-2022 The Pybricks Authors

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
PYBRICKSDEV = pybricksdev
METADATA = $(PBTOP)/tools/metadata.py
OPENOCD ?= openocd
OPENOCD_CONFIG ?= openocd_stm32$(PB_MCU_SERIES_LCASE).cfg
TEXT0_ADDR ?= 0x08000000

CFLAGS_MCU_F0 = -mthumb -mtune=cortex-m0 -mcpu=cortex-m0  -msoft-float
CFLAGS_MCU_F4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_MCU_L4 = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS_WARN = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-maybe-uninitialized
CFLAGS = $(INC) -std=c99 -nostdlib -fshort-enums $(CFLAGS_MCU_$(PB_MCU_SERIES)) $(CFLAGS_WARN) $(COPT) $(CFLAGS_EXTRA)
$(BUILD)/lib/libm/%.o: CFLAGS += -Wno-sign-compare
$(BUILD)/lib/stm32lib/%.o: CFLAGS += -Wno-sign-compare
$(BUILD)/lib/STM32_USB_Device_Library/%.o: CFLAGS += -Wno-sign-compare

# define external oscillator frequency
CFLAGS += -DHSE_VALUE=$(PB_MCU_EXT_OSC_HZ)

# linker scripts
LD_FILES = $(PBIO_PLATFORM).ld $(PBTOP)/bricks/stm32/common.ld

LDFLAGS = $(addprefix -T,$(LD_FILES)) -Wl,-Map=$@.map -Wl,--cref -Wl,--gc-sections

# avoid doubles
CFLAGS += -fsingle-precision-constant -Wdouble-promotion

# Tune for Debugging or Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -Og -ggdb
else
CFLAGS += -Os -DNDEBUG -flto
CFLAGS += -fdata-sections -ffunction-sections
endif

# Required for STM32 library
CFLAGS += -D$(PB_CMSIS_MCU)

CFLAGS += -DSTM32_H='<stm32$(PB_MCU_SERIES_LCASE)xx.h>'
CFLAGS += -DSTM32_HAL_H='<stm32$(PB_MCU_SERIES_LCASE)xx_hal.h>'

MPY_CROSS = ../../micropython/mpy-cross/mpy-cross

LIBS = "$(shell $(CC) $(CFLAGS) -print-libgcc-file-name)"

SRC_C = $(addprefix bricks/stm32/,\
	main.c \
	mphalport.c \
	)

# Extra core MicroPython files

# NB: Since we are using MicroPython's build system, files in the micropython/
# directory have the micropython/ prefix excluded. It is very important to do
# it that way since there is special handling of certain files that will break
# if we don't do it this way. So we need to be very careful about name clashes
# between the top level directory and the micropython/ subdirectory.

SRC_C += $(addprefix shared/,\
	libc/string0.c \
	readline/readline.c \
	runtime/gchelper_native.c \
	runtime/interrupt_char.c \
	runtime/pyexec.c \
	runtime/stdout_helpers.c \
	runtime/sys_stdio_mphal.c \
	)

SRC_S = \
	lib/pbio/platform/$(PBIO_PLATFORM)/startup.s \

ifeq ($(PB_MCU_SERIES),F0)
	SRC_S += shared/runtime/gchelper_m0.s
else
	SRC_S += shared/runtime/gchelper_m3.s
endif

# Pybricks modules

PYBRICKS_PYBRICKS_SRC_C = $(addprefix pybricks/,\
	common/pb_type_battery.c \
	common/pb_type_charger.c \
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
	common/pb_type_system.c \
	experimental/pb_module_experimental.c \
	geometry/pb_module_geometry.c \
	geometry/pb_type_matrix.c \
	hubs/pb_module_hubs.c \
	hubs/pb_type_cityhub.c \
	hubs/pb_type_essentialhub.c \
	hubs/pb_type_technichub.c \
	hubs/pb_type_movehub.c \
	hubs/pb_type_primehub.c \
	iodevices/pb_module_iodevices.c \
	iodevices/pb_type_iodevices_analogsensor.c \
	iodevices/pb_type_iodevices_ev3devsensor.c \
	iodevices/pb_type_iodevices_i2cdevice.c \
	iodevices/pb_type_iodevices_lumpdevice.c \
	iodevices/pb_type_iodevices_lwp3device.c \
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
	pupdevices/pb_type_pupdevices_colorlightmatrix.c \
	pupdevices/pb_type_pupdevices_colorsensor.c \
	pupdevices/pb_type_pupdevices_forcesensor.c \
	pupdevices/pb_type_pupdevices_infraredsensor.c \
	pupdevices/pb_type_pupdevices_light.c \
	pupdevices/pb_type_pupdevices_pfmotor.c \
	pupdevices/pb_type_pupdevices_remote.c \
	pupdevices/pb_type_pupdevices_tiltsensor.c \
	pupdevices/pb_type_pupdevices_ultrasonicsensor.c \
	pybricks.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_drivebase.c \
	robotics/pb_type_spikebase.c \
	tools/pb_module_tools.c \
	tools/pb_type_stopwatch.c \
	util_mp/pb_obj_helper.c \
	util_mp/pb_type_enum.c \
	util_pb/pb_color_map.c \
	util_pb/pb_conversions.c \
	util_pb/pb_device_stm32.c \
	util_pb/pb_error.c \
	util_pb/pb_task.c \
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

# Contiki

CONTIKI_SRC_C = $(addprefix lib/contiki-core/,\
	lib/list.c \
	lib/memb.c \
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
ifeq ($(PB_MCU_SERIES),F4)
HAL_SRC_C := $(filter-out %xx_hal_uart_ex.c, $(HAL_SRC_C))
endif
ifneq ($(PB_MCU_SERIES),F4)
HAL_SRC_C := $(filter-out %xx_hal_fmpi2c.c, $(HAL_SRC_C))
endif
ifneq ($(PB_MCU_SERIES),L4)
HAL_SRC_C := $(filter-out %xx_ll_lpuart.c, $(HAL_SRC_C))
endif
ifneq ($(PB_LIB_STM32_USB_DEVICE),1)
HAL_SRC_C := $(filter-out %xx_hal_pcd_ex.c, $(HAL_SRC_C))
HAL_SRC_C := $(filter-out %xx_hal_pcd.c, $(HAL_SRC_C))
HAL_SRC_C := $(filter-out %xx_ll_usb.c, $(HAL_SRC_C))
endif

# Ring buffer

LWRB_SRC_C = lib/lwrb/src/lwrb/lwrb.c

# Pybricks I/O library

PBIO_SRC_C = $(addprefix lib/pbio/,\
	drv/adc/adc_stm32_hal.c \
	drv/adc/adc_stm32f0.c \
	drv/battery/battery_adc.c \
	drv/block_device/block_device_flash_stm32.c \
	drv/block_device/block_device_w25qxx_stm32.c \
	drv/bluetooth/bluetooth_btstack_control_gpio.c \
	drv/bluetooth/bluetooth_btstack_run_loop_contiki.c \
	drv/bluetooth/bluetooth_btstack_uart_block_stm32_hal.c \
	drv/bluetooth/bluetooth_btstack.c \
	drv/bluetooth/bluetooth_init_cc2564C_1.4.c \
	drv/bluetooth/bluetooth_stm32_bluenrg.c \
	drv/bluetooth/bluetooth_stm32_cc2640.c \
	drv/bluetooth/pybricks_service_server.c \
	drv/button/button_gpio.c \
	drv/button/button_resistor_ladder.c \
	drv/charger/charger_mp2639a.c \
	drv/clock/clock_stm32.c \
	drv/core.c \
	drv/counter/counter_core.c \
	drv/counter/counter_lpf2.c \
	drv/counter/counter_stm32f0_gpio_quad_enc.c \
	drv/gpio/gpio_stm32f0.c \
	drv/gpio/gpio_stm32f4.c \
	drv/gpio/gpio_stm32l4.c \
	drv/imu/imu_lsm6ds3tr_c_stm32.c \
	drv/ioport/ioport_lpf2.c \
	drv/led/led_array_pwm.c \
	drv/led/led_array.c \
	drv/led/led_core.c \
	drv/led/led_dual.c \
	drv/led/led_pwm.c \
	drv/motor_driver/motor_driver_hbridge_pwm.c \
	drv/pwm/pwm_core.c \
	drv/pwm/pwm_lp50xx_stm32.c \
	drv/pwm/pwm_stm32_tim.c \
	drv/pwm/pwm_tlc5955_stm32.c \
	drv/reset/reset_stm32.c \
	drv/resistor_ladder/resistor_ladder.c \
	drv/sound/sound_stm32_hal_dac.c \
	drv/uart/uart_stm32f0.c \
	drv/uart/uart_stm32f4_ll_irq.c \
	drv/uart/uart_stm32l4_ll_dma.c \
	drv/usb/usb_stm32.c \
	drv/watchdog/watchdog_stm32.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	src/angle.c \
	src/battery.c \
	src/color/conversion.c \
	src/control.c \
	src/control_settings.c \
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
	src/motor/servo_settings.c \
	src/observer.c \
	src/parent.c \
	src/protocol/lwp3.c \
	src/protocol/nus.c \
	src/protocol/pybricks.c \
	src/servo.c \
	src/tacho.c \
	src/task.c \
	src/trajectory.c \
	src/uartdev.c \
	src/util.c \
	sys/battery.c \
	sys/bluetooth.c \
	sys/command.c \
	sys/core.c \
	sys/hmi.c \
	sys/io_ports.c \
	sys/light_matrix.c \
	sys/light.c \
	sys/main.c \
	sys/program_load.c \
	sys/program_stop.c \
	sys/status.c \
	sys/supervisor.c \
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

ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
	INC += -I$(PBTOP)/lib/pbio/drv/usb/stm32_usbd
endif

SRC_STM32_USB_DEV += $(addprefix lib/pbio/drv/usb/stm32_usbd/,\
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
OBJ += $(addprefix $(BUILD)/, $(LWRB_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(PBIO_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_LIBM:.c=.o))
ifeq ($(PB_LIB_STM32_USB_DEVICE),1)
OBJ += $(addprefix $(BUILD)/, $(SRC_STM32_USB_DEV:.c=.o))
endif

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(PYBRICKS_PYBRICKS_SRC_C)
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

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

FW_VERSION := $(shell $(GIT) describe --tags --dirty --always --exclude "@pybricks/*")

$(BUILD)/firmware.elf: $(LD_FILES) $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)
	$(Q)$(SIZE) -A $@

# firmware blob without main.mpy or checksum - use as base for appending other .mpy
$(BUILD)/firmware-base.bin: $(BUILD)/firmware.elf
	$(ECHO) "BIN creating firmware base file"
	$(Q)$(OBJCOPY) -O binary -j .isr_vector -j .text -j .data -j .name $^ $@
	$(ECHO) "`wc -c < $@` bytes"

$(BUILD)/firmware.metadata.json: $(BUILD)/firmware.elf $(METADATA)
	$(ECHO) "META creating firmware metadata"
	$(Q)$(METADATA) $(FW_VERSION) $(PBIO_PLATFORM) $<.map $@

# firmware.zip file
ZIP_FILES := \
	$(BUILD)/firmware-base.bin \
	$(BUILD)/firmware.metadata.json \
	ReadMe_OSS.txt \

ifeq ($(PB_FW_ZIP_INCLUDE_MAIN_MPY),1)
ZIP_FILES += main.py
endif

$(BUILD)/firmware.zip: $(ZIP_FILES)
	$(ECHO) "ZIP creating firmware package"
	$(Q)$(ZIP) -j $@ $^

# firmware in DFU format
$(BUILD)/%.dfu: $(BUILD)/%-base.bin
	$(ECHO) "DFU Create $@"
	$(Q)$(PYTHON) $(DFU) -b $(TEXT0_ADDR):$< $@

deploy: $(BUILD)/firmware.zip
	$(Q)$(PYBRICKSDEV) flash $< --name $(PBIO_PLATFORM)

deploy-openocd: $(BUILD)/firmware-base.bin
	$(ECHO) "Writing $< to the board via ST-LINK using OpenOCD"
	$(Q)$(OPENOCD) -f $(OPENOCD_CONFIG) -c "stm_flash $< $(TEXT0_ADDR)"

include $(TOP)/py/mkrules.mk
