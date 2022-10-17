# SPDX-License-Identifier: MIT
# Copyright (c) 2022 The Pybricks Authors

###############################
# Standard MicroPython config #
###############################

# Enable/disable modules and 3rd-party libs to be included in interpreter

# Build 32-bit binaries on a 64-bit host
MICROPY_FORCE_32BIT = 0

# This variable can take the following values:
#  0 - no readline, just simple stdin input
#  1 - use MicroPython version of readline
MICROPY_USE_READLINE = 1

# btree module using Berkeley DB 1.xx
MICROPY_PY_BTREE = 1

# _thread module using pthreads
MICROPY_PY_THREAD = 1

# Subset of CPython termios module
MICROPY_PY_TERMIOS = 1

# Subset of CPython socket module
MICROPY_PY_SOCKET = 1

# ffi module requires libffi (libffi-dev Debian package)
MICROPY_PY_FFI = 1

# ussl module requires one of the TLS libraries below
MICROPY_PY_USSL = 1
# axTLS has minimal size but implements only a subset of modern TLS
# functionality, so may have problems with some servers.
MICROPY_SSL_AXTLS = 1
# mbedTLS is more up to date and complete implementation, but also
# more bloated.
MICROPY_SSL_MBEDTLS = 0

# jni module requires JVM/JNI
MICROPY_PY_JNI = 0

# Avoid using system libraries, use copies bundled with MicroPython
# as submodules (currently affects only libffi).
MICROPY_STANDALONE = 0

######################
# Pybricks additions #
######################

USER_C_MODULES = ../../..

INC += -I../../..
INC += -I../../../lib/contiki-core
INC += -I../../../lib/lego
INC += -I../../../lib/pbio/include
INC += -I../../../lib/pbio/platform/virtual_hub

# Sources and libraries common to all pybricks bricks
PBIO_PLATFORM = virtual_hub
include ../../../bricks/_common/sources.mk

EXTMOD_SRC_C += $(PYBRICKS_PYBRICKS_SRC_C)
LIB_SRC_C += $(CONTIKI_SRC_C)
LIB_SRC_C += $(PBIO_SRC_C)

# realtime library for timer signals
LIB += -lrt

# embedded Python
EMBEDDED_PYTHON ?= python3.10
PYTHON_CONFIG := $(EMBEDDED_PYTHON)-config

INC += $(shell $(PYTHON_CONFIG) --includes)
LDFLAGS += -rdynamic $(shell $(PYTHON_CONFIG) --ldflags --embed)
