# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2021 The Pybricks Authors

# ensure git submodule checked out
ifeq ("$(wildcard ../../btstack/README.md)","")
$(info GIT cloning btstack submodule)
$(info $(shell cd ../../.. && git submodule update --checkout --init lib/btstack))
ifeq ("$(wildcard ../../btstack/README.md)","")
$(error failed)
endif
endif

# output
ifeq ($(COVERAGE),1)
BUILD_DIR = build-coverage
else
BUILD_DIR = build
endif
BUILD_PREFIX = $(BUILD_DIR)/lib/pbio/test
PROG = $(BUILD_DIR)/test-pbio

# verbose
ifeq ("$(origin V)", "command line")
BUILD_VERBOSE=$(V)
endif
ifndef BUILD_VERBOSE
BUILD_VERBOSE = 0
endif
ifeq ($(BUILD_VERBOSE),0)
Q = @
else
Q =
endif

all: $(PROG)

# using tinytest from upstream MicroPython
TINY_TEST_DIR = ../../../micropython/lib/tinytest
TINY_TEST_INC = -I$(TINY_TEST_DIR)
TINY_TEST_SRC = $(TINY_TEST_DIR)/tinytest.c

# pbio depedency
CONTIKI_DIR = ../../contiki-core
CONTIKI_INC = -I$(CONTIKI_DIR)
CONTIKI_SRC = $(addprefix $(CONTIKI_DIR)/, \
	lib/list.c \
	lib/memb.c \
	sys/autostart.c \
	sys/etimer.c \
	sys/process.c \
	sys/timer.c \
	)

# pbio depedency
LEGO_DIR = ../../lego
LEGO_INC = -I$(LEGO_DIR)
LEGO_SRC =

# pbio depedency
FIXMATH_DIR = ../../libfixmath
FIXMATH_INC = -I$(FIXMATH_DIR)/libfixmath
FIXMATH_SRC = $(shell find $(FIXMATH_DIR)/libfixmath -name "*.c")

# pbio depedency
LWRB_DIR = ../../lwrb/src
LWRB_INC = -I$(LWRB_DIR)/include
LWRB_SRC = $(shell find $(LWRB_DIR)/lwrb -name "*.c")

# pbio depedency
BTSTACK_DIR = ../../btstack
BTSTACK_INC = -I$(BTSTACK_DIR)/src -I$(BTSTACK_DIR)/chipset/cc256x -I$(BTSTACK_DIR)/platform/posix
BTSTACK_SRC = $(addprefix $(BTSTACK_DIR)/src/,\
	btstack_linked_list.c \
	btstack_run_loop.c \
	btstack_util.c \
)

BTSTACK_SRC += $(addprefix $(BTSTACK_DIR)/src/,\
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

BTSTACK_SRC += $(addprefix $(BTSTACK_DIR)/src/ble/,\
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

BTSTACK_SRC += $(addprefix $(BTSTACK_DIR)/chipset/cc256x/,\
	btstack_chipset_cc256x.c \
	)

BTSTACK_SRC += $(addprefix $(BTSTACK_DIR)/platform/posix/,\
	hci_dump_posix_stdout.c \
	)

# pbio library
PBIO_DIR = ..
PBIO_INC = -I$(PBIO_DIR)/include -I$(PBIO_DIR)
PBIO_SRC = \
	$(shell find $(PBIO_DIR)/drv ! -wholename "*/ev3dev_stretch/*" -and ! -wholename "*/stm32_usbd/*" -and -name "*.c") \
	$(shell find $(PBIO_DIR)/src -name "*.c") \
	$(shell find $(PBIO_DIR)/sys -name "*.c") \
	$(shell find $(PBIO_DIR)/src/motor -name "*.c") \

# tests
TEST_INC = -I.
TEST_SRC = $(shell find . -name "*.c")

# generated files

GATT_FILES := $(addprefix $(PBIO_DIR)/drv/bluetooth/,\
	pybricks_service.gatt \
	)

GATT_H_FILES := $(addprefix $(BUILD_DIR)/genhdr/, $(notdir $(GATT_FILES:.gatt=.h)))

$(BUILD_PREFIX)/$(PBIO_DIR)/drv/bluetooth/bluetooth_btstack.o: $(GATT_H_FILES)

$(BUILD_DIR)/genhdr/%.h: $(PBIO_DIR)/drv/bluetooth/%.gatt
	$(Q)mkdir -p $(dir $@)
	$(Q)python3 $(BTSTACK_DIR)/tool/compile_gatt.py $< $@


CFLAGS += -std=gnu99 -g -O0 -Wall -Werror
CFLAGS += $(TINY_TEST_INC) $(CONTIKI_INC) $(LEGO_INC) $(FIXMATH_INC) $(LWRB_INC) $(BTSTACK_INC) $(PBIO_INC) $(TEST_INC)
CFLAGS += -I$(BUILD_DIR)
CFLAGS += -DPBIO_TEST_BUILD=1

ifeq ($(COVERAGE),1)
CFLAGS += --coverage
endif

SRC = $(TINY_TEST_SRC) $(CONTIKI_SRC) $(LEGO_SRC) $(FIXMATH_SRC) $(LWRB_SRC) $(BTSTACK_SRC) $(PBIO_SRC) $(TEST_SRC)
DEP = $(addprefix $(BUILD_PREFIX)/,$(SRC:.c=.d))
OBJ = $(addprefix $(BUILD_PREFIX)/,$(SRC:.c=.o))

clean:
	$(Q)rm -rf $(BUILD_DIR)
ifneq ($(COVERAGE),1)
	$(Q)$(MAKE) COVERAGE=1 clean
endif

$(BUILD_PREFIX)/%.d: %.c
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) $(CFLAGS) -MM -MT $(patsubst %.d,%.o,$@) $< > $@

-include $(DEP)

$(BUILD_PREFIX)/%.o: %.c $(BUILD_PREFIX)/%.d Makefile
	$(Q)mkdir -p $(dir $@)
	@echo CC $<
	$(Q)$(CC) -c $(CFLAGS) -o $@ $<

$(PROG): $(OBJ)
	$(Q)$(CC) $(CFLAGS) -o $@ $^

build-coverage/lcov.info: Makefile $(SRC)
	$(Q)$(MAKE) COVERAGE=1
	./build-coverage/test-pbio
	lcov --capture --output-file $@ \
		--directory build-coverage/lib/pbio/drv \
		--directory build-coverage/lib/pbio/src \
		--directory build-coverage/lib/pbio/sys \
		--exclude **/btstack/** \
		--exclude **/contiki-core/** \
		--exclude **/libfixmath/**

coverage-html: build-coverage/lcov.info
	$(Q)genhtml $^ --output-directory build-coverage/html
