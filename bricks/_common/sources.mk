# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2023 The Pybricks Authors

# This file contains the sources common to all Pybricks MicroPython ports.

# Ring buffer

LWRB_SRC_C = lib/lwrb/src/lwrb/lwrb.c

# Pybricks modules

PYBRICKS_PYBRICKS_SRC_C = $(addprefix pybricks/,\
	common/pb_type_battery.c \
	common/pb_type_ble.c \
	common/pb_type_charger.c \
	common/pb_type_colorlight_external.c \
	common/pb_type_colorlight_internal.c \
	common/pb_type_control.c \
	common/pb_type_device.c \
	common/pb_type_imu.c \
	common/pb_type_keypad.c \
	common/pb_type_lightarray.c \
	common/pb_type_lightmatrix_fonts.c \
	common/pb_type_lightmatrix.c \
	common/pb_type_logger.c \
	common/pb_type_motor_model.c \
	common/pb_type_motor.c \
	common/pb_type_speaker.c \
	common/pb_type_system.c \
	ev3devices/pb_module_ev3devices.c \
	ev3devices/pb_type_ev3devices_colorsensor.c \
	ev3devices/pb_type_ev3devices_gyrosensor.c \
	ev3devices/pb_type_ev3devices_infraredsensor.c \
	ev3devices/pb_type_ev3devices_touchsensor.c \
	ev3devices/pb_type_ev3devices_ultrasonicsensor.c \
	experimental/pb_module_experimental.c \
	hubs/pb_module_hubs.c \
	hubs/pb_type_cityhub.c \
	hubs/pb_type_essentialhub.c \
	hubs/pb_type_ev3brick.c \
	hubs/pb_type_movehub.c \
	hubs/pb_type_nxtbrick.c \
	hubs/pb_type_primehub.c \
	hubs/pb_type_technichub.c \
	hubs/pb_type_virtualhub.c \
	iodevices/pb_module_iodevices.c \
	iodevices/pb_type_iodevices_analogsensor.c \
	iodevices/pb_type_i2c_device.c \
	iodevices/pb_type_iodevices_lwp3device.c \
	iodevices/pb_type_iodevices_pupdevice.c \
	iodevices/pb_type_iodevices_xbox_controller.c \
	iodevices/pb_type_uart_device.c \
	nxtdevices/pb_module_nxtdevices.c \
	nxtdevices/pb_type_nxtdevices_colorsensor.c \
	nxtdevices/pb_type_nxtdevices_energymeter.c \
	nxtdevices/pb_type_nxtdevices_lightsensor.c \
	nxtdevices/pb_type_nxtdevices_soundsensor.c \
	nxtdevices/pb_type_nxtdevices_temperaturesensor.c \
	nxtdevices/pb_type_nxtdevices_touchsensor.c \
	nxtdevices/pb_type_nxtdevices_ultrasonicsensor.c \
	nxtdevices/pb_type_nxtdevices_vernieradapter.c \
	parameters/pb_module_parameters.c \
	parameters/pb_type_axis.c \
	parameters/pb_type_button.c \
	parameters/pb_type_color.c \
	parameters/pb_type_direction.c \
	parameters/pb_type_font.c \
	parameters/pb_type_icon.c \
	parameters/pb_type_image.c \
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
	pupdevices/pb_type_pupdevices_tiltsensor.c \
	pupdevices/pb_type_pupdevices_ultrasonicsensor.c \
	pybricks.c \
	robotics/pb_module_robotics.c \
	robotics/pb_type_car.c \
	robotics/pb_type_drivebase.c \
	robotics/pb_type_spikebase.c \
	tools/pb_module_tools.c \
	tools/pb_type_app_data.c \
	tools/pb_type_async.c \
	tools/pb_type_matrix.c \
	tools/pb_type_stopwatch.c \
	tools/pb_type_task.c \
	util_mp/pb_obj_helper.c \
	util_mp/pb_type_enum.c \
	util_pb/pb_color_map.c \
	util_pb/pb_conversions.c \
	util_pb/pb_error.c \
	)

# Pybricks I/O library

PBIO_SRC_C = $(addprefix lib/pbio/,\
	drv/adc/adc_ev3.c \
	drv/adc/adc_stm32_hal.c \
	drv/adc/adc_stm32f0.c \
	drv/battery/battery_adc.c \
	drv/battery/battery_ev3.c \
	drv/battery/battery_nxt.c \
	drv/battery/battery_test.c \
	drv/block_device/block_device_ev3.c \
	drv/block_device/block_device_flash_stm32.c \
	drv/block_device/block_device_test.c \
	drv/block_device/block_device_w25qxx_stm32.c \
	drv/bluetooth/bluetooth.c \
	drv/bluetooth/bluetooth_btstack_stm32_hal.c \
	drv/bluetooth/bluetooth_btstack.c \
	drv/bluetooth/bluetooth_btstack_ev3.c \
	drv/bluetooth/bluetooth_simulation.c \
	drv/bluetooth/bluetooth_stm32_bluenrg.c \
	drv/bluetooth/bluetooth_stm32_cc2640.c \
	drv/bluetooth/firmware/bluetooth_init_cc2564C_1.4.c \
	drv/bluetooth/firmware/bluetooth_init_cc2560.c \
	drv/bluetooth/firmware/bluetooth_init_cc2560a.c \
	drv/bluetooth/pybricks_service_server.c \
	drv/button/button_gpio.c \
	drv/button/button_nxt.c \
	drv/button/button_resistor_ladder.c \
	drv/button/button_test.c \
	drv/cache/cache_ev3.c \
	drv/charger/charger_mp2639a.c \
	drv/clock/clock_ev3.c \
	drv/clock/clock_linux.c \
	drv/clock/clock_none.c \
	drv/clock/clock_nxt.c \
	drv/clock/clock_stm32.c \
	drv/clock/clock_test.c \
	drv/core.c \
	drv/counter/counter_ev3.c \
	drv/counter/counter_nxt.c \
	drv/counter/counter_stm32f0_gpio_quad_enc.c \
	drv/display/display_ev3.c \
	drv/display/display_virtual.c \
	drv/gpio/gpio_ev3.c \
	drv/gpio/gpio_stm32f0.c \
	drv/gpio/gpio_stm32f4.c \
	drv/gpio/gpio_stm32l4.c \
	drv/gpio/gpio_virtual.c \
	drv/i2c/i2c_ev3.c \
	drv/imu/imu_lsm6ds3tr_c_stm32.c \
	drv/ioport/ioport.c \
	drv/led/led_array_pwm.c \
	drv/led/led_array.c \
	drv/led/led_core.c \
	drv/led/led_dual.c \
	drv/led/led_pwm.c \
	drv/motor_driver/motor_driver_ev3.c \
	drv/motor_driver/motor_driver_hbridge_pwm.c \
	drv/motor_driver/motor_driver_nxt.c \
	drv/motor_driver/motor_driver_virtual_simulation.c \
	drv/pwm/pwm_core.c \
	drv/pwm/pwm_ev3.c \
	drv/pwm/pwm_lp50xx_stm32.c \
	drv/pwm/pwm_stm32_tim.c \
	drv/pwm/pwm_test.c \
	drv/pwm/pwm_tlc5955_stm32.c \
	drv/random/random_adc.c \
	drv/random/random_stm32_hal.c \
	drv/reset/reset_ev3.c \
	drv/reset/reset_nxt.c \
	drv/reset/reset_stm32.c \
	drv/resistor_ladder/resistor_ladder.c \
	drv/rproc/rproc_ev3.c \
	drv/sound/beep_sampled.c \
	drv/sound/sound_ev3.c \
	drv/sound/sound_nxt.c \
	drv/sound/sound_stm32_hal_dac.c \
	drv/stack/stack_embedded.c \
	drv/uart/uart_debug_first_port.c \
	drv/uart/uart_ev3_pru.c \
	drv/uart/uart_ev3.c \
	drv/uart/uart_stm32f0.c \
	drv/uart/uart_stm32f4_ll_irq.c \
	drv/uart/uart_stm32l4_ll_dma.c \
	drv/usb/usb.c \
	drv/usb/usb_common_desc.c \
	drv/usb/usb_ev3.c \
	drv/usb/usb_nxt.c \
	drv/usb/usb_simulation.c \
	drv/usb/usb_stm32.c \
	drv/watchdog/watchdog_ev3.c \
	drv/watchdog/watchdog_stm32.c \
	platform/$(PBIO_PLATFORM)/platform.c \
	src/angle.c \
	src/battery.c \
	src/busy_count.c \
	src/color/conversion.c \
	src/color/util.c \
	src/control_settings.c \
	src/control.c \
	src/dcmotor.c \
	src/differentiator.c \
	src/drivebase.c \
	src/error.c \
	src/geometry.c \
	src/image/font_liberationsans_regular_14.c \
	src/image/font_terminus_normal_16.c \
	src/image/font_mono_8x5_8.c \
	src/image/image.c \
	src/imu.c \
	src/int_math.c \
	src/integrator.c \
	src/light/animation.c \
	src/light/color_light.c \
	src/light/light_matrix.c \
	src/logger.c \
	src/main.c \
	src/motor_process.c \
	src/motor/servo_settings.c \
	src/observer.c \
	src/os.c \
	src/parent.c \
	src/port_dcm_ev3.c \
	src/port_dcm_pup.c \
	src/port_lump.c \
	src/port.c \
	src/protocol/nus.c \
	src/protocol/pybricks.c \
	src/servo.c \
	src/tacho.c \
	src/trajectory.c \
	src/util.c \
	sys/battery_temp.c \
	sys/battery.c \
	sys/command.c \
	sys/core.c \
	sys/hmi_env_mpy.c \
	sys/hmi_lcd.c \
	sys/hmi_pup.c \
	sys/hmi_none.c \
	sys/host.c \
	sys/light.c \
	sys/main.c \
	sys/program_stop.c \
	sys/status.c \
	sys/storage_settings.c \
	sys/storage.c \
	sys/telemetry.c \
	)

# LEGO specification library

LEGO_SPEC_SRC_C = $(addprefix lib/lego/,\
	device.c \
	)

# MicroPython math library

SRC_LIBM = $(addprefix lib/libm/,\
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

ifeq ($(SUPPORTS_HARDWARE_FP_SINGLE),1)
SRC_LIBM += lib/libm/thumb_vfp_sqrtf.c
else
SRC_LIBM += lib/libm/ef_sqrt.c
endif
