#all:
#	gcc -Wall -O0 -mcpu=arm7tdmi-s -mapcs -mthumb-interwork -o flash.o flash.c -nostartfiles -nodefaultlibs -nostdlib -Wl,-e,main
#	objcopy -Obinary -j.text flash.o flash.bin
#	objdump --disassemble-all -bbinary -marm7tdmi flash.bin > flash.asm
#

CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy

TARGET = flash_driver.bin

all:
	$(CC) -W -Wall -Os -msoft-float -mcpu=arm7tdmi -mapcs -c -o flash.o flash.c
	$(AS) --warn -mfpu=softfpa -mcpu=arm7tdmi -mapcs-32 -o crt0.o crt0.s
	$(LD) -Os --gc-sections crt0.o flash.o -o flash.elf
	$(OBJCOPY) -O binary flash.elf $(TARGET)
	chmod -x $(TARGET)
	rm -f flash.o crt0.o flash.elf

clean:
	rm -f flash.o crt0.o flash.elf $(TARGET)
