Pybricks IO Library
===================

The Pybricks IO library is a low-level C library that provides a hardware
abstraction layer for Pybricks devices.

Currently, this library powers Pybricks MicroPython. In the future, this library
could be made into a stand-alone project to be used as the base for other
higher level libraries.

Directory Structure
-------------------

The `config` directory contains platform-specific configuration files.

The `include` directory contains the common API that is implemented on all
platforms.

The `src` directory contains subdirectories with platform-specific
implementations of the common API.
