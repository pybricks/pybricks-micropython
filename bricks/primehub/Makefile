# SPDX-License-Identifier: MIT
# Copyright (c) 2026 The Pybricks Authors

# Wrapper makefile that builds all primehub variants and combines them into a
# single multi-variant firmware.zip.

VARIANTS = primehub_f4 primehub_h5

BUILD = build
PYTHON = python3
COMBINE = ../../tools/combine_firmware.py
FLASH = $(PYTHON) ../../tools/flash
VARIANT_ZIPS = $(foreach v,$(VARIANTS),../$(v)/build/firmware.zip)

all: $(BUILD)/firmware.zip

# Check out git submodules serially first, using one variant, so that the
# parallel variant sub-makes below don't race each other trying to clone the
# same submodules at the same time (which fails).
submodules:
	$(MAKE) -C ../$(firstword $(VARIANTS)) submodules

$(VARIANTS): submodules
	$(MAKE) -C ../$@

# Always rebuild: the variant sub-makes decide what is up to date.
$(BUILD)/firmware.zip: $(VARIANTS)
	mkdir -p $(BUILD)
	$(PYTHON) $(COMBINE) $@ $(VARIANT_ZIPS)

deploy: $(BUILD)/firmware.zip
	$(FLASH) $< --name primehub

clean:
	for variant in $(VARIANTS); do $(MAKE) -C ../$$variant clean; done
	rm -rf $(BUILD)

.PHONY: all clean deploy submodules $(VARIANTS)
