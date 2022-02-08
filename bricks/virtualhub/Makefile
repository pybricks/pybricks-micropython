# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

# Need a default target so that make can be called without specifying one.
all:

# The virtual hub is a unix port variant, so pass everything to the upstream
# MicroPython makefile.
%:
	$(MAKE) -C ../../micropython/ports/unix \
		VARIANT=virtualhub \
		VARIANT_DIR=../../../bricks/virtualhub \
		BUILD=../../../bricks/virtualhub/build \
		PROG=../../../bricks/virtualhub/build/virtualhub-micropython \
		$@
