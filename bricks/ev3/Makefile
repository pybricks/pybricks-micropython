PBIO_PLATFORM = ev3
PB_MCU_FAMILY = TIAM1808

# Disable some options. Can get rid of this once linker file and compile flags
# are properly set up.
DEBUG = 2

include ../_common/arm_none_eabi.mk

$(BUILD)/uImage: $(BUILD)/firmware-base.bin
	mkimage -C none -A arm -T kernel -O linux -a 0xC0008000 -e 0xC0008000 -d $< $@

uImage: $(BUILD)/uImage
