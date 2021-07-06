<!-- Refer to https://keepachangelog.com/en/1.0.0/ for guidance. -->

# Changelog

## [Unreleased]

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

## Prerelease

Prerelease changes are documented at [support#48].


<!-- let's try to keep this list sorted -->
[issue#21]: https://github.com/pybricks/pybricks-micropython/issues/21
[issue#49]: https://github.com/pybricks/pybricks-micropython/issues/49
[pull#57]: https://github.com/pybricks/pybricks-micropython/pull/57
[issue#58]: https://github.com/pybricks/pybricks-micropython/issues/58
[support#48]: https://github.com/pybricks/support/issues/48
[support#52]: https://github.com/pybricks/support/issues/52
[support#186]: https://github.com/pybricks/support/issues/186
[support#321]: https://github.com/pybricks/support/issues/321
[support#347]: https://github.com/pybricks/support/issues/347
[support#352]: https://github.com/pybricks/support/issues/352
[support#361]: https://github.com/pybricks/support/issues/361
[support#379]: https://github.com/pybricks/support/issues/379

[Unreleased]: https://github.com/pybricks/pybricks-micropython/compare/v3.1.0a1...HEAD
[3.1.0a1]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0...v3.1.0a1
[3.0.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0c1...v3.0.0
