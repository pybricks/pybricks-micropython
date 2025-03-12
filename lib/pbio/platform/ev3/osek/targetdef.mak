TARGET := leJOS_EV3

C_SOURCES := \
	systick.c \
        i2c.c \
        init.c \
        adc_sensors.c \
        digi_sensors.c \
        ev3_motors.c \
        power.c \
	drivers/timer.c \
        drivers/interrupt.c \
        drivers/cpu.c \
        drivers/psc.c \
        drivers/syscfg.c \
	drivers/ehrpwm.c \
	drivers/gpio.c \
	drivers/spi.c \
	drivers/ecap.c \
        ninja/adc.c \
        ninja/gpio.c \
        ninja/spi.c \
        ninja/pininfo.c \
	ninja/led.c \
	ninja/motor.c \
	ninja/button.c \
