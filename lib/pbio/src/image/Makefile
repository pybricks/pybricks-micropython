# Use this Makefile to generate font files.

FONTCONVERT = ../../tools/fontconvert.py

LIBERATION_LICENSE = "OFL-1.1-RFN"
LIBERATION_COPYRIGHT = \
	--copyright "Digitized data copyright (c) 2010 Google Corporation" \
	--copyright "  with Reserved Font Arimo, Tinos and Cousine." \
	--copyright "Copyright (c) 2012 Red Hat, Inc." \
	--copyright "  with Reserved Font Name Liberation."
LIBERATION_SANS_REGULAR = /usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf

TERMINUS_LICENSE = "OFL-1.1-RFN"
TERMINUS_COPYRIGHT = \
	--copyright "Copyright (c) 2010-2014 Dimitar Toshkov Zhekov," \
	--copyright '  with Reserved Font Name "Terminus Font".'
TERMINUS_NORMAL = /usr/share/fonts/opentype/terminus/terminus-normal.otb

NXOS_LICENSE = "GPL-2.0-only"
NXOS_COPYRIGHT = \
	--copyright "Copyright (c) 2008 the NxOS developers"

all: font_liberationsans_regular_14.c font_terminus_normal_16.c font_mono_8x5_8.c

font_liberationsans_regular_14.c: $(LIBERATION_SANS_REGULAR) $(FONTCONVERT)
	python3 $(FONTCONVERT) $< 14 --license $(LIBERATION_LICENSE) $(LIBERATION_COPYRIGHT) > $@

font_terminus_normal_16.c: $(TERMINUS_NORMAL) $(FONTCONVERT)
	python3 $(FONTCONVERT) $< 16 --license $(TERMINUS_LICENSE) $(TERMINUS_COPYRIGHT) > $@

font_mono_8x5_8.c: mono_8x5.png $(FONTCONVERT)
	python3 $(FONTCONVERT) --image $< 8 --license $(NXOS_LICENSE) $(NXOS_COPYRIGHT) > $@

.DELETE_ON_ERROR:
