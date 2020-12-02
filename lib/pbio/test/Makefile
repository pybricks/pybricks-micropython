# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

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

# using tinytest from upstream MicroPython
TINY_TEST_DIR = ../../../micropython/lib/tinytest
TINY_TEST_INC = -I$(TINY_TEST_DIR)
TINY_TEST_SRC = $(TINY_TEST_DIR)/tinytest.c

# pbio depedency
CONTIKI_DIR = ../../contiki-core
CONTIKI_INC = -I$(CONTIKI_DIR)
CONTIKI_SRC = $(addprefix $(CONTIKI_DIR)/, \
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
BTSTACK_DIR = ../../btstack
BTSTACK_INC = -I$(BTSTACK_DIR)/src
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

# pbio library
PBIO_DIR = ..
PBIO_INC = -I$(PBIO_DIR)/include -I$(PBIO_DIR)
PBIO_SRC = \
	$(shell find $(PBIO_DIR)/drv/ ! -wholename "*/ev3dev_stretch/*" -and -name "*.c") \
	$(shell find $(PBIO_DIR)/src -name "*.c") \
	$(shell find $(PBIO_DIR)/sys -name "*.c") \

# tests
TEST_INC = -I.
TEST_SRC = $(shell find . -name "*.c")


CFLAGS += -std=gnu99 -g -O0 -Wall -Werror -fshort-enums
CFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections
CFLAGS += $(TINY_TEST_INC) $(CONTIKI_INC) $(LEGO_INC) $(FIXMATH_INC) $(BTSTACK_INC) $(PBIO_INC) $(TEST_INC)
CFLAGS += -DPBIO_TEST_BUILD=1

ifeq ($(COVERAGE),1)
CFLAGS += --coverage
endif

BUILD_PREFIX = $(BUILD_DIR)/lib/pbio/test
SRC = $(TINY_TEST_SRC) $(CONTIKI_SRC) $(LEGO_SRC) $(FIXMATH_SRC) $(BTSTACK_SRC) $(PBIO_SRC) $(TEST_SRC)
DEP = $(addprefix $(BUILD_PREFIX)/,$(SRC:.c=.d))
OBJ = $(addprefix $(BUILD_PREFIX)/,$(SRC:.c=.o))

all: $(PROG)

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
	$(Q)$(CC) $(CFLAGS) -o $@ $^ -lrt

build-coverage/lcov.info: Makefile $(SRC)
	$(Q)$(MAKE) COVERAGE=1
	./build-coverage/test-pbio
	lcov --capture --output-file $@ \
		--directory build-coverage/lib/pbio/drv \
		--directory build-coverage/lib/pbio/src \
		--directory build-coverage/lib/pbio/sys \
		--exclude **/contiki-core/** \
		--exclude **/libfixmath/**

coverage-html: build-coverage/lcov.info
	$(Q)genhtml $^ --output-directory build-coverage/html
