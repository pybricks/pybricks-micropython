Pybricks I/O Library
====================

The Pybricks IO library is a C library that provides a cooperative-multitasking
hardware abstraction layer for Pybricks devices.

Currently, this library powers Pybricks MicroPython. In the future, this library
could be made into a stand-alone project.

Directory Structure
-------------------

The `doc` directory contains the documentation build configuration. Online docs
are available at <https://docs.pybricks.com/projects/pbio/>.

The `drv` directory contains "drivers" that act as a hardware abstraction layer
for the various platform-specific hardware.

The `include/pbdrv` directory contains the public header files for the drivers
(`drv` directory). Generally, these header files should not be used outside of
this library.

The `include/pbio` directory contains the public header files for the main
library (`src` directory). This is the API for user programs.

The `include/pbsys` directory contains the public header files for the operating
system-level library functions (`sys` directory).

The `platform` directory contains platform-specific code.

The `src` directory contains the main library source code.

The `sys` directory contains the core "operating system" code.

The `test` directory contains unit tests for the library.
