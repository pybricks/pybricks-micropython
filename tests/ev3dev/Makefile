# SPDX-License-Identifier: MIT
# Copyright (c) 2019 David Lechner

CFLAGS += $(shell pkg-config --cflags grx-3.0)
LDFLAGS += $(shell pkg-config --libs grx-3.0)

CFLAGS += -fPIC
LDFLAGS += -shared

libgrx-3.0-vdriver-test.so: grx-plugin.o
	$(LD) $(LDFLAGS) -o $@ $<

grx-plugin.o: Makefile
