.SUFFIXES:

default: def_target

include environment.mak
include targetdef.mak

LIB_TARGET := $(TARGET).a

S_OBJECTS := $(S_SOURCES:.s=.o)
C_OBJECTS := $(C_SOURCES:.c=.o)

C_OPTIMISATION_FLAGS = -O0

CFLAGS = $(C_OPTIMISATION_FLAGS) -c -std=gnu99 -mcpu=arm926ej-s -g -Dgcc -Dam1808 \
	-Winline -Wall -Werror-implicit-function-declaration \
	-I. -Iinclude -Iinclude/hw -Iinclude/armv5 -Iinclude/armv5/am1808 -ffunction-sections -fdata-sections -fomit-frame-pointer -ffreestanding

ASFLAGS = -mfpu=softfpa

def_target: all

ALL_TARGETS := $(LIB_TARGET)

.PHONY: all
all: BuildMessage $(ALL_TARGETS)

PHONY: TargetMessage
TargetMessage:
	@echo ""
	@echo "Building: $(ALL_TARGETS)"
	@echo ""
	@echo "C sources: $(C_SOURCES) to $(C_OBJECTS)"
	@echo ""

PHONY: BuildMessage
BuildMessage: TargetMessage EnvironmentMessage

$(LIB_TARGET): $(C_OBJECTS) $(S_OBJECTS)
	@echo "Building static library $@"
	$(AR) rcs -o $@ $+

%.o: %.s
	@echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -o $@ $< 

%.o: %.c
	@echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -o $@ $< 

.PHONY: clean
clean:  
	@echo "Removing all Objects"
	@rm -f $(S_OBJECTS) $(C_OBJECTS) *.o
	@echo "Removing target"
	@rm -f $(ALL_TARGETS)

