# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

# Need a default target so that make can be called without specifying one.
all:

# place debug build in separate folder so we don't have to remember to clean and rebuild
ifeq ($(DEBUG),1)
BUILD_DIR = build-debug
else
BUILD_DIR = build
endif

# The virtual hub is a unix port variant, so pass everything to the upstream
# MicroPython makefile.
%:
	$(MAKE) -C ../../micropython/ports/unix \
		VARIANT=virtualhub \
		VARIANT_DIR=../../../bricks/virtualhub \
		BUILD=../../../bricks/virtualhub/$(BUILD_DIR) \
		PROG=../../../bricks/virtualhub/$(BUILD_DIR)/virtualhub-micropython \
		QSTR_DEFS=../../../bricks/pybricks_qstrdefs.h \
		$@
