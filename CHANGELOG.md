<!-- Refer to https://keepachangelog.com/en/1.0.0/ for guidance. -->

# Changelog

## [Unreleased]

### Added
- Added special location in firmware for storing hub name ([support#52]). Note:
  Support will need to be added to tools separately to make use of this.

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
[support#48]: https://github.com/pybricks/support/issues/48
[support#52]: https://github.com/pybricks/support/issues/52
[support#321]: https://github.com/pybricks/support/issues/321
[support#347]: https://github.com/pybricks/support/issues/347
[support#352]: https://github.com/pybricks/support/issues/352
[support#361]: https://github.com/pybricks/support/issues/361

[Unreleased]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0...HEAD
[3.0.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0c1...v3.0.0
