<!-- Refer to https://keepachangelog.com/en/1.0.0/ for guidance. -->

# Changelog

## [Unreleased]

### Added
- Support for LEGO Technic Color Light Matrix ([support#440]).
- Support for LEGO UART devices with a new battery power flag. This is
  required to support the new LEGO Technic Color Light Matrix ([support#440]).
- Support for the SPIKE Essential hub/Technic Small hub ([support#439]).

### Fixed
- Fixed Ultrasonic Sensor and Color Sensor turning off when a
  user script ends ([support#456]).

### Changed:
- Updated to MicroPython v1.17.

[support#439]: https://github.com/pybricks/support/issues/439
[support#440]: https://github.com/pybricks/support/issues/440
[support#456]: https://github.com/pybricks/support/issues/456

## [3.1.0a4] - 2021-08-30

### Added
- Enabled builtin `bytearray` ([pull#60]).
- Enabled `ustruct` module ([pull#60]).
- Added alpha support for dual boot installation on the SPIKE Prime Hub.
- Added `pybricks.experimental.hello_world` function to make it easier for
  new contributors to experiment with Pybricks using C code.
- Added ability to import the `main.mpy` that is embedded in the firmware from
  a download and run program ([support#408]).
- Added `pybricks.iodevices.LWP3Device` to communicate with a device that supports
  the LEGO Wireless Protocol 3.0.00 ([pull#68])

### Changed
- Move Hub Bluetooth optimizations to reduce firmware size ([issue#49]).
- Disabled `pybricks.iodevices` module on Move Hub to reduce firmware size.
- Improvements to `pybricks.pupdevices.Remote`:
  - Check if a remote is already connected before attempting to create a new
    connection.
  - Rename first argument from `address` to `name` to match documentation.
  - Implement connecting by name.
  - Add `name()` method.
  - Add `light` attribute.

[issue#49]: https://github.com/pybricks/pybricks-micropython/issues/49
[pull#60]: https://github.com/pybricks/pybricks-micropython/pull/60
[pull#68]: https://github.com/pybricks/pybricks-micropython/pull/68
[support#408]: https://github.com/pybricks/support/issues/408

## [3.1.0a3] - 2021-07-19

### Added
- Added `reset_angle=False` keyword argument to `Motor()` class.
  This makes resetting the angle optional, allowing to maintain absolute
  positioning for robots with gears ([support#389]).

### Changed
- Moved MicroPython `math` module to `umath` to be consistent with other
  MicroPython modules.

### Fixed
- Fixed City hub not always powering off on shutdown ([support#385]).
- Fixed Move hub turning back on after shutdown ([support#386]).

[support#385]: https://github.com/pybricks/support/issues/385
[support#386]: https://github.com/pybricks/support/issues/386
[support#389]: https://github.com/pybricks/support/issues/389

## [3.1.0a2] - 2021-07-06

### Added
- Support for reading button state on Technic Hub, City Hub, and Move Hub.
- Support for reading hub name on all Powered Up hubs.
- Enable MicroPython slicing feature on Technic Hub, City Hub, Prime Hub.
- Added pystone benchmark tests for all hubs.
- Added `hub.system.shutdown()` for graceful shutdown ([support#58]).

### Changed
- Move `Remote` class from `experimental` to `pupdevices`.
- Move `Remote.pressed()` to `Remote.buttons.pressed()`.
- Remove `pybricks.experimental` module on Move Hub.
- Increased Move Hub heap from 6K to 7K due to recently freed RAM ([pull#57]).
- Changed `hub.system.reset(action)` to accept `2` only ([support#379]). This
  is technically a breaking change, although this method was never officially
  released or documented. With this change, entering firmware update mode
  remains possible for backwards-compatibility, but unsafe power off methods
  are removed.

### Fixed
- Fix Bluetooth not disconnecting when shutting down and button is still pressed.
- Fix I/O ports not powering off when shutting down and button is still pressed.
- Fix version number only showing git hash and not v3.x.

[pull#57]: https://github.com/pybricks/pybricks-micropython/pull/57
[support#58]: https://github.com/pybricks/pybricks-micropython/issues/58
[support#379]: https://github.com/pybricks/support/issues/379

## [3.1.0a1] - 2021-06-23

### Added
- Added PnP ID characteristic to Device Information Service and to advertising
  data ([issue#49]).
- Added special location in firmware for storing hub name ([support#52]). Note:
  Support will need to be added to tools separately to make use of this.
- Added configuration option to run a simplified version of motor PID control.
  It is activated on the Move Hub to reduce build size. Motor performance on
  the other hubs is unaffected.
- Experimental support for the Powered Up remote control ([support#186]).

### Changed
- Updated MicroPython to v1.16.
- Simplified the Powered Up UART Protocol host implementation. Unused device
  properties are no longer stored in order to reduce flash and RAM
  usage ([pull#57]).

[issue#49]: https://github.com/pybricks/pybricks-micropython/issues/49
[support#52]: https://github.com/pybricks/support/issues/52
[support#186]: https://github.com/pybricks/support/issues/186

## [3.0.0] - 2021-06-08

### Added
- Added `Hub.system` attribute ([support#321]).

### Changed
- `Hub.reset()` moved to `Hub.system.reset()` ([support#321]).
- Motor methods now raise `OSError` with `uerrno.EBUSY` instead of `uerrno.EPERM`
  if the motor is currently being used and the operation cannot be completed.

### Fixed
- Fixed resetting motor angle while holding position causes movement ([support#352]).
- Fixed `DriveBase` not resetting during initialization ([issue#21]).
- Fixed <kbd>Ctrl</kbd>+<kbd>C</kbd> stopping REPL when first started ([support#347]).
- Fixed UART I/O devices retuning uninitialized data ([support#361]).

[issue#21]: https://github.com/pybricks/pybricks-micropython/issues/21
[support#321]: https://github.com/pybricks/support/issues/321
[support#347]: https://github.com/pybricks/support/issues/347
[support#352]: https://github.com/pybricks/support/issues/352
[support#361]: https://github.com/pybricks/support/issues/361

## Prerelease

Prerelease changes are documented at [support#48].

[support#48]: https://github.com/pybricks/support/issues/48


<!-- diff links for headers -->
[Unreleased]: https://github.com/pybricks/pybricks-micropython/compare/v3.1.0a4...HEAD
[3.1.0a4]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a3...v3.1.0a4
[3.1.0a3]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a2...v3.1.0a3
[3.1.0a2]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a1...v3.1.0a2
[3.1.0a1]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0...v3.1.0a1
[3.0.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0c1...v3.0.0
