# SPDX-License-Identifier: MIT
# Copyright 2019 David Lechner <david@pybricks.com>

# output
ifeq ($(COVERAGE),1)
BUILD_DIR = build-coverage
else
BUILD_DIR = build
endif
PROG = $(BUILD_DIR)/test-pbio


# using tinytest from upstream MicroPython
TINY_TEST_DIR = ../../../../../lib/tinytest
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
FIXMATH_DIR = ../../libfixmath
FIXMATH_INC = -I$(FIXMATH_DIR)/libfixmath
FIXMATH_SRC = $(shell find $(FIXMATH_DIR)/libfixmath -name "*.c")

# pbio library
PBIO_DIR = ..
PBIO_INC = -I$(PBIO_DIR)/include -I$(PBIO_DIR)
PBIO_SRC = $(shell find $(PBIO_DIR)/src -name "*.c")

# tests
TEST_INC = -I.
TEST_SRC = $(shell find . -name "*.c")


CFLAGS += -std=gnu99 -g -O0 -Wall -Werror
CFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections
CFLAGS += $(TINY_TEST_INC) $(CONTIKI_INC) $(FIXMATH_INC) $(PBIO_INC) $(TEST_INC)

ifeq ($(COVERAGE),1)
CFLAGS += --coverage
endif

BUILD_PREFIX = $(BUILD_DIR)/ports/pybricks/lib/pbio/test
SRC = $(TINY_TEST_SRC) $(CONTIKI_SRC) $(FIXMATH_SRC) $(PBIO_SRC) $(TEST_SRC)
DEP = $(addprefix $(BUILD_PREFIX)/,$(SRC:.c=.d))
OBJ = $(addprefix $(BUILD_PREFIX)/,$(SRC:.c=.o))

all: $(PROG)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_PREFIX)/%.d: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MM -MT $(patsubst %.d,%.o,$@) $< > $@

-include $(DEP)

$(BUILD_PREFIX)/%.o: %.c $(BUILD_PREFIX)/%.d
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

$(PROG): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

build-coverage/coverage.info: Makefile $(SRC)
	$(MAKE) COVERAGE=1
	./build-coverage/test-pbio
	lcov --capture --output-file $@ \
		--directory build-coverage/ports/pybricks/lib/pbio/src

coverage-html: build-coverage/coverage.info
	genhtml $^ --output-directory build-coverage/html
