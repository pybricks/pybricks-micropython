TARGET_NAME = OSEK-$(TARGET)

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc $(DEFINES)
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump

LOADADDR = 0xC1000000

INCLUDEDIR = -I$(EV3OSEK_ROOT)/leJOS_EV3/src/ev3 -I$(EV3OSEK_ROOT)/OSEK_EV3/kernel -I$(EV3OSEK_ROOT)/leJOS_EV3/src/ev3/include/hw -I$(EV3OSEK_ROOT)/leJOS_EV3/src/ev3/include -I$(EV3OSEK_ROOT)/leJOS_EV3/src/ev3/include/armv5 \
		-I$(EV3OSEK_ROOT)/leJOS_EV3/src/ev3/include/armv5/am1808 -I$(EV3OSEK_ROOT)/OSEK_EV3/include -I$(EV3OSEK_ROOT)/OSEK_EV3/config/ARM926EJ-S -I$(EV3OSEK_ROOT)/OSEK_EV3/config/ARM926EJ-S/lego_ev3 -I$(EV3OSEK_ROOT)/ECRobot -I$(EV3OSEK_ROOT)/leJOS_EV3/src/ev3/ninja -I./ 
LDFLAGS = -nostdlib -T$(EV3OSEK_ROOT)/OSEK_EV3/OSEK.ld
CPUFLAG = -mcpu=arm926ej-s
STATIC_LIBS = $(EV3OSEK_ROOT)/ECRobot/ECRobot.a $(EV3OSEK_ROOT)/newlib/libc.a $(EV3OSEK_ROOT)/leJOS_EV3/src/ev3/leJOS_EV3.a

ELF = $(TARGET_NAME).elf
BIN = $(TARGET_NAME).bin
ASM = $(TARGET_NAME).asm
BOOT_CMD = boot.cmd
BOOT_SCR = boot.scr


TOPPERS_CFG_SOURCE = kernel_cfg.c
TOPPERS_CFG_HEADER = kernel_id.h
TOPPERS_OSEK_ROOT = $(EV3OSEK_ROOT)/OSEK_EV3/sg/

C_SRC_FILES = $(EV3OSEK_ROOT)/OSEK_EV3/OSEK.c $(EV3OSEK_ROOT)/newlib/syscalls.c $(wildcard $(EV3OSEK_ROOT)/OSEK_EV3/kernel/*.c) $(wildcard $(EV3OSEK_ROOT)/OSEK_EV3/config/ARM926EJ-S/*.c) $(wildcard $(EV3OSEK_ROOT)/OSEK_EV3/config/ARM926EJ-S/lego_ev3/*.c) \
        $(TOPPERS_CFG_SOURCE) $(TARGET_SOURCES)
        
S_SRC_FILES = $(wildcard $(EV3OSEK_ROOT)/OSEK_EV3/*.S) $(wildcard $(EV3OSEK_ROOT)/OSEK_EV3/config/ARM926EJ-S/*.S)
OBJECTS := $(C_SRC_FILES:.c=.o) $(S_SRC_FILES:.S=.o) 


all: implementation.oil $(OBJECTS) ECRobot
	@#always recompile osctl.c in order to reevaluate compiler flags for Hook-defines
	@touch $(EV3OSEK_ROOT)/OSEK_EV3/kernel/osctl.c
	@echo "Linking $(ELF)..." 
	$(LD) $(LDFLAGS) $(OBJECTS) $(STATIC_LIBS) -o $(ELF) -lgcc --library-path=$(EV3OSEK_ROOT)/newlib
	@echo "Creating $(BIN)..."
	$(OBJCOPY) -O binary $(ELF) $(BIN)
	@echo "Looking for entry point address..."
	$(OBJDUMP) -d $(ELF) > $(ASM)
	@echo "Creating $(BOOT_CMD)..."
	echo "fatload mmc 0 0xC1000000 $(BIN)" > $(BOOT_CMD)
	echo "go 0xC1000000" >> $(BOOT_CMD)
	@echo "Creating $(BOOT_SCR)..."
	mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n 'Execute $(BIN)' -d $(BOOT_CMD) $(BOOT_SCR)
	@echo "Moving application to SD-Card if you've created /mnt/sd and device /dev/mmcblk0p1 exists";
	@if [ -d "/mnt/sd/" ]; then \
            sudo mount /dev/mmcblk0p1 /mnt/sd; \
            sudo rm -f /mnt/sd/$(BIN); \
            sudo rm -f /mnt/sd/$(BOOT_SCR); \
            sudo cp $(BIN) /mnt/sd/; \
            sudo cp $(BOOT_SCR) /mnt/sd/; \
            sudo umount /mnt/sd; \
        fi


$(TOPPERS_CFG_SOURCE) $(TOPPERS_CFG_HEADER) implementation.oil : $(TOPPERS_OSEK_OIL_SOURCE)
	@echo "Generating OSEK kernel config files from $(TOPPERS_OSEK_OIL_SOURCE)"
	wine $(TOPPERS_OSEK_ROOT)sg.exe $(TOPPERS_OSEK_OIL_SOURCE) -os=ECC2 -I$(TOPPERS_OSEK_ROOT)impl_oil -template=$(TOPPERS_OSEK_ROOT)lego_nxt.sgt

ECRobot:
	cd $(EV3OSEK_ROOT)/ECRobot/ && make all
	
leJOS:
	cd $(EV3OSEK_ROOT)/leJOS_EV3/src/ev3 && make all
	
%.o: %.S
	@echo "Compiling $<..."
	$(CC) $(CPUFLAG) -c -g -std=gnu99 -mthumb-interwork -ffreestanding -fdata-sections -ffunction-sections -Wall -Dgcc -Dam1808 -O0 $(INCLUDEDIR) -o $@ $<
	
%.o: %.c
	@echo "Compiling $<..." 
	$(CC) $(CPUFLAG) -c -g -fomit-frame-pointer -std=gnu99 -mthumb-interwork -ffreestanding -fdata-sections -ffunction-sections -Wall -Dgcc -Dam1808 -O0 $(INCLUDEDIR) -o $@ $<

clean:
	rm $(wildcard *.o) $(wildcard */*.o) $(wildcard */*/*.o) $(wildcard */*/*/*.o) *.asm boot.cmd boot.scr *.bin *.elf implementation.oil kernel_cfg.c kernel_id.h
