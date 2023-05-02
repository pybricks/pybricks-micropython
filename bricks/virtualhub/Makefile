# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

# Need a default target so that make can be called without specifying one.
all:

# place coverage/debug build in separate folder so we don't have to remember to clean and rebuild
ifeq ($(COVERAGE),1)
BUILD_DIR = build-coverage
export CFLAGS = --coverage
export LDFLAGS = --coverage
else
ifeq ($(DEBUG),1)
BUILD_DIR = build-debug
export COPT = -O0
else
BUILD_DIR = build
endif
endif

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
PBTOP := $(patsubst %/bricks/virtualhub/Makefile,%,$(mkfile_path))
ifeq ("$(wildcard $(PBTOP)/micropython/lib/micropython-lib/README.md)","")
$(info GIT cloning micropython-lib submodule)
$(info $(shell cd $(PBTOP)/micropython && git submodule update --init lib/micropython-lib))
ifeq ("$(wildcard $(PBTOP)/micropython/lib/micropython-lib/README.md)","")
$(error failed)
endif
endif

# Include frozen manifest only if there is anything to freeze.
ifneq ("$(wildcard ../../bricks/_common/modules/*.py)","")
FROZEN_MANIFEST ?= ../../../bricks/_common/manifest.py
else
FROZEN_MANIFEST =
endif

# The virtual hub is a unix port variant, so pass everything to the upstream
# MicroPython makefile.
%:
	$(MAKE) -C ../../micropython/ports/unix \
		VARIANT=virtualhub \
		VARIANT_DIR=../../../bricks/virtualhub \
		BUILD=../../../bricks/virtualhub/$(BUILD_DIR) \
		PROG=../../../bricks/virtualhub/$(BUILD_DIR)/virtualhub-micropython \
		FROZEN_MANIFEST=$(FROZEN_MANIFEST) \
		QSTR_DEFS=../../../bricks/_common/qstrdefs.h \
		QSTR_GLOBAL_DEPENDENCIES=../../../bricks/virtualhub/mpconfigvariant.h \
		$@
