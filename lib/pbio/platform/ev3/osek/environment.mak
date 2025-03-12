  GCC_VERSION := 5.2.0
  COMP_PATH   := /
  LIBPREFIX   := /usr/arm-none-eabi/lib
  LIBC        := ../../../newlib/libc.a

  TARGET_PREFIX := arm-none-eabi
  CC       := $(TARGET_PREFIX)-gcc
  AR       := $(TARGET_PREFIX)-ar
  OBJCOPY  := $(TARGET_PREFIX)-objcopy

PHONY: EnvironmentMessage
EnvironmentMessage:
	@echo " CC      $(CC)"
	@echo " AS      $(AS)"
	@echo " AR      $(AR)"
	@echo " OBJCOPY $(OBJCOPY)"
	@echo " "

