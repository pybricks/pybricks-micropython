Pybricks I/O Library
====================

The Pybricks IO library is a C library that provides a cooperative-multitasking
hardware abstraction layer for Pybricks devices.

Currently, this library powers Pybricks MicroPython. In the future, this library
could be made into a stand-alone project.

Directory Structure
-------------------

The `doc` directory contains the doxygen build configuration. TODO: post doc
build output online somewhere.

The `drv` directory contains subdirectories with platform-specific
implementations of the common API.

The `include/pbiodrv` directory contains the common I/O driver API that must be
implemented for each platform. These header files are not intended to be used
outside of this library.

The `include/pbio` directory contains the public header files for the main library.

The `include/pbsys` directory contains the public header files for the
system-level library functions.

The `platform` directory contains platform-specific code.

The `src` directory contains the main library source code.

The `sys` directory contains platform-specific code used when this library is
used as the "operating system" for a programmable brick.
