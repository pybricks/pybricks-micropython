<!-- Refer to https://keepachangelog.com/en/1.0.0/ for guidance. -->

# Changelog

## [Unreleased]

### Added
- Added support for `PBIO_PYBRICKS_COMMAND_REBOOT_TO_UPDATE_MODE` Pybricks Profile BLE command.

### Changed
- The Pybricks Code stop button will force the program to exit even if the user
  catches the `SystemExit` exception ([pybricks-micropython#117]).

### Fixed
- Fixed connecting `Remote` on BOOST move hub ([support#793]).

### Removed
- Removed `hub.system.reset()` method.

[pybricks-micropython#117]: https://github.com/pybricks/pybricks-micropython/pull/117
[support#793]: https://github.com/pybricks/support/issues/793

## [3.2.0b5] - 2022-11-11

### Added
- Added `DriveBase.stalled()` for convenient stall detection.
- Added `DriveBase.done()` for convenient completion detection, which is
  practical when combined with `wait=False`.
- Added `Motor.done()` for convenient completion detection, which is
  practical when combined with `wait=False`. Especially on Move Hub, which
  does not have the control attribute enabled.

### Fixed
- Fixed brief hub freeze on `pybricks.common.Logger.save()` when not connected
  to the computer ([support#738]).
- Fixed drive base stall flags being set while not stalled ([support#767]).
- Fixed `Motor.run_target` raising exception for short moves ([support#786]).

[support#738]: https://github.com/pybricks/support/issues/738
[support#767]: https://github.com/pybricks/support/issues/767
[support#786]: https://github.com/pybricks/support/issues/786

## [3.2.0b4] - 2022-10-21

### Added
- Indicate that the hub is shutting down by quickly flashing the hub light for
  half a second. This makes it easier to see when you can stop pressing the
  button.
- Indicate that the SPIKE Prime hub is booting and shutting down by fading
  the stop sign in and out.
- Implemented iterator protocol on `geometry.Matrix` class.
- Added support for multi-file projects ([pybricks-micropython#115]).
- Added new `System.storage()` API ([support#85]).

### Changed
- Battery full indication (green light) comes on earlier ([support#647]).
- New indication for over-charging battery (blinking green light).
- On Move Hub, City Hub, and Technic Hub, programs can now be restarted with
  the button after downloading them. They are saved on shutdown.
- Improved program download process. Reduces the likelihood of getting errors
  about incompatible .mpy files when accidentally entering characters in the
  terminal window when no program is active.
- On Prime Hub and Essential Hub, there is no longer a wait time after boot
  before you can start programs.
- On Prime Hub and Essential Hub, the user program is now stored in a section
  of the external flash that is not used by any file system of other known
  firmwares, in order to avoid compatibility issues when changing firmware.
- Restored the `Motor.speed()` method and `DriveBase` equivalent to provide
  speed as a numerical derivative of the motor position, instead of a
  model-based estimate. For most use cases, this is a more intuitive result
  because this speed value is not affected by mechanical load.
- When using the REPL, everything from all Pybricks modules was automatically
  imported for convenience. Now, MicroPython modules are also automatically
  imported ([support#741]).
- Updated Bluetooth to [Pybricks Profile v1.2.0][pp1.2.0].
- Bluetooth now uses random private address instead of static public address
  ([support#600]).

### Fixed
- Fixed motors going out of sync when starting program ([support#679]).
- Fixed motor torque signal overflowing under load ([support#729]).
- Fixed city hub turning back on after shutdown ([support#692]).
- Fixed IMU I2C bus lockup on SPIKE hubs ([support#232]).
- Fixed REPL history corrupt after soft reset ([support#699]).
- Fixed "ValueError: incompatible .mpy file" when pressing the button when
  there is no program yet ([support#599]).

[pp1.2.0]: https://github.com/pybricks/technical-info/blob/master/pybricks-ble-profile.md#profile-v120
[pybricks-micropython#115]: https://github.com/pybricks/pybricks-micropython/pull/115
[support#85]: https://github.com/pybricks/support/issues/85
[support#232]: https://github.com/pybricks/support/issues/232
[support#599]: https://github.com/pybricks/support/issues/599
[support#600]: https://github.com/pybricks/support/issues/600
[support#647]: https://github.com/pybricks/support/issues/647
[support#679]: https://github.com/pybricks/support/issues/679
[support#692]: https://github.com/pybricks/support/issues/692
[support#699]: https://github.com/pybricks/support/issues/699
[support#729]: https://github.com/pybricks/support/issues/729
[support#741]: https://github.com/pybricks/support/issues/741

## [3.2.0b3] - 2022-07-20

### Fixed
- Fix integral control not working properly due to mistakes introduced while
  converting the controllers to use millidegrees.

### Changed
- `Motor.run_time` no longer raises an exception for negative time values.
  Negative times are now treated as zero, thus producing a stationary
  trajectory.

## [3.2.0b2] - 2022-07-06

### Added
- Added `Motor.stalled()`. It can detect stall during speed and position
  control (`run`, `run_angle`, ...) just like `Motor.control.stalled()`, but
  it also detects stall for `dc()` command when the user controls the voltage
  directly.

### Fixed
- Fixed motor not stopping at the end of `run_until_stalled` ([support#662]).
- Fixed incorrect battery current reading on Technic hub ([support#665]).
- When the motor was pushed backwards while stalled, the `control.stalled()`
  was inadvertently cleared because a nonzero speed was detected. This is fixed
  by checking the intended direction as well.
- Fixed I/O devices not syncing at high baud rate.
- Fixed `ENODEV` error while device connection manager is busy ([support#674]).

### Changed
- Reworked internal motor model that is used to estimate speed. This results
  in better speed estimation at low speeds, which makes PID control smoother.
- The `Motor.speed()` method and `DriveBase` equivalents now provide the
  estimated speed instead of the value reported by the motor. This is generally
  more responsive.
- Overhauled the control code to make it smaller and more numerically robust
  while using higher position resolution where it is available.
- Changed drive base default speed to go a little slower.
- Updated MicroPython to v1.19.

[support#662]: https://github.com/pybricks/support/issues/662
[support#665]: https://github.com/pybricks/support/issues/665
[support#674]: https://github.com/pybricks/support/issues/674

## [3.2.0b1] - 2022-06-03

### Added
- Added `Stop.NONE` as `then` option for motors. This allows subsequent
  motor and drive base commands to transition without stopping.
- Added `Stop.COAST_SMART` as `then` option for motors. This still coasts the
  motor, but it keeps track of the previously used position target. When a new
  relative angle command is given (e.g. rotate 90 degrees), it uses that
  position as the starting point. This avoids accumulation of errors when using
  relative angles in succession.
- Made motor deceleration configurable separately from acceleration.
- Enabled `ujson` module.
- Added ability to use more than one `DriveBase` in the same script.
- Added support for battery charging on Prime and essential hubs.

### Changed
- Changed how `DriveBases` and `Motor` classes can be used together.
  Previously, an individual motor could not be used when a drive base used it.
  From now on, devices can always be used. If they were already in use by
  something else, that other class will just be stopped (coast).
- Changed how unexpected motor problems are handled, such as a cable being
  unplugged while it was running. Previously, this raised a `SystemExit` no
  matter which motor was unplugged. Now it will return an `OSError` with
  `ENODEV`, which is consistent with trying to initialize a motor that isn't
  there. The `Motor` class must be initialized again to use the motor again.
- Changing settings while a motor is moving no longer raises an exception. Some
  settings will not take effect until a new motor command is given.
- Disabled `Motor.control` and `Motor.log` on Move Hub to save space.
- Changed LED color calibration on Prime hub to make yellow less green.
- Updated to upstream MicroPython v1.18.
- Changed imu.acceleration() units to mm/s/s ([pybricks-micropython#88]) for
  Move Hub, Technic Hub, and Prime Hub.

### Fixed
- Fixed color calibration on Powered Up remote control ([support#424]).
- Fixed 3x3 Light Matrix colors with hue > 255 not working correctly ([support#619]).

[pybricks-micropython#88]: https://github.com/pybricks/pybricks-micropython/issues/88
[support#424]: https://github.com/pybricks/support/issues/424
[support#619]: https://github.com/pybricks/support/issues/619

## [3.1.0] - 2021-12-16

### Changed
- Renamed new `DCMotor.dc_settings()` method to `DCMotor.settings()` ([support#536]).

### Fixed
- Fixed direction for `DriveBase.turn()` and `Drivebase.curve()` for some
  arguments ([support#535]).
- Fixed `then=Stop.COAST` not working in `DriveBase` methods ([support#535]).

[support#535]: https://github.com/pybricks/support/issues/535
[support#536]: https://github.com/pybricks/support/issues/536

## [3.1.0c1] - 2021-11-19

### Added
- Added `DriveBase.curve()` method to drive an arc segment.
- Added `then` and `wait` arguments to `DriveBase` methods ([support#57]).

### Changed
- Dropped `integral_range` argument from `Control.pid()`. This setting was
  ineffective and never used. When set incorrectly, the motor could get stuck
  for certain combinations of `kp` and `ki`.
- Improved motor behavior for cases with low-speed, low-load, but high
  inertia ([support#366]).
- Changed how the duty cycle limit is set for `Motor` and `DCMotor`. It is now
  set as a voltage limit via a dedicated method, instead of `Motor.control`.

### Fixed
- Fixed `then=Stop.COAST` being ignored in most motor commands.
- Fixed `brake()`/`light.off()` not working on Move hub I/O port C ([support#501]).
- Fixed `Remote()` failing to connect when hub is connected to 2019 or newer
  MacBooks ([support#397]).
- Fixed intermittent improper detection of hot-plugged I/O devices ([support#500]).
- A program now stops when a `Motor` is unplugged while it is running, instead
  of getting in a bad state.

[support#57]: https://github.com/pybricks/support/issues/57
[support#366]: https://github.com/pybricks/support/issues/366
[support#397]: https://github.com/pybricks/support/issues/397
[support#500]: https://github.com/pybricks/support/issues/500
[support#501]: https://github.com/pybricks/support/issues/501

## [3.1.0b1] - 2021-09-21

### Added
- Support for LEGO Technic Color Light Matrix ([support#440]).
- Support for LEGO UART devices with a new battery power flag. This is
  required to support the new LEGO Technic Color Light Matrix ([support#440]).
- Support for the SPIKE Essential hub/Technic Small hub ([support#439]).

### Fixed
- Fixed Ultrasonic Sensor and Color Sensor turning off when a
  user script ends ([support#456]).
- Hub reset due to watchdog timer when writing data to UART I/O device
  ([support#304]).
- City/Technic hubs not connecting via Bluetooth on macOS 12 ([support#489]).

### Changed
- Updated to MicroPython v1.17.

[support#304]: https://github.com/pybricks/support/issues/304
[support#439]: https://github.com/pybricks/support/issues/439
[support#440]: https://github.com/pybricks/support/issues/440
[support#456]: https://github.com/pybricks/support/issues/456
[support#489]: https://github.com/pybricks/support/issues/489

## [3.1.0a4] - 2021-08-30

### Added
- Enabled builtin `bytearray` ([pybricks-micropython#60]).
- Enabled `ustruct` module ([pybricks-micropython#60]).
- Added alpha support for dual boot installation on the SPIKE Prime Hub.
- Added `pybricks.experimental.hello_world` function to make it easier for
  new contributors to experiment with Pybricks using C code.
- Added ability to import the `main.mpy` that is embedded in the firmware from
  a download and run program ([support#408]).
- Added `pybricks.iodevices.LWP3Device` to communicate with a device that supports
  the LEGO Wireless Protocol 3.0.00 ([pybricks-micropython#68])

### Changed
- Move Hub Bluetooth optimizations to reduce firmware size ([pybricks-micropython#49]).
- Disabled `pybricks.iodevices` module on Move Hub to reduce firmware size.
- Improvements to `pybricks.pupdevices.Remote`:
  - Check if a remote is already connected before attempting to create a new
    connection.
  - Rename first argument from `address` to `name` to match documentation.
  - Implement connecting by name.
  - Add `name()` method.
  - Add `light` attribute.

[pybricks-micropython#49]: https://github.com/pybricks/pybricks-micropython/issues/49
[pybricks-micropython#60]: https://github.com/pybricks/pybricks-micropython/pull/60
[pybricks-micropython#68]: https://github.com/pybricks/pybricks-micropython/pull/68
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
- Increased Move Hub heap from 6K to 7K due to recently freed RAM ([pybricks-micropython#57]).
- Changed `hub.system.reset(action)` to accept `2` only ([support#379]). This
  is technically a breaking change, although this method was never officially
  released or documented. With this change, entering firmware update mode
  remains possible for backwards-compatibility, but unsafe power off methods
  are removed.

### Fixed
- Fix Bluetooth not disconnecting when shutting down and button is still pressed.
- Fix I/O ports not powering off when shutting down and button is still pressed.
- Fix version number only showing git hash and not v3.x.

[pybricks-micropython#57]: https://github.com/pybricks/pybricks-micropython/pull/57
[support#58]: https://github.com/pybricks/pybricks-micropython/issues/58
[support#379]: https://github.com/pybricks/support/issues/379

## [3.1.0a1] - 2021-06-23

### Added
- Added PnP ID characteristic to Device Information Service and to advertising
  data ([pybricks-micropython#49]).
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
  usage ([pybricks-micropython#57]).

[pybricks-micropython#49]: https://github.com/pybricks/pybricks-micropython/issues/49
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
- Fixed `DriveBase` not resetting during initialization ([pybricks-micropython#21]).
- Fixed <kbd>Ctrl</kbd>+<kbd>C</kbd> stopping REPL when first started ([support#347]).
- Fixed UART I/O devices retuning uninitialized data ([support#361]).

[pybricks-micropython#21]: https://github.com/pybricks/pybricks-micropython/issues/21
[support#321]: https://github.com/pybricks/support/issues/321
[support#347]: https://github.com/pybricks/support/issues/347
[support#352]: https://github.com/pybricks/support/issues/352
[support#361]: https://github.com/pybricks/support/issues/361

## Prerelease

Prerelease changes are documented at [support#48].

[support#48]: https://github.com/pybricks/support/issues/48


<!-- diff links for headers -->
[Unreleased]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0b5...HEAD
[3.2.0b5]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0b4...v3.2.0b5
[3.2.0b4]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0b3...v3.2.0b4
[3.2.0b3]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0b2...v3.2.0b3
[3.2.0b2]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0b1...v3.2.0b2
[3.2.0b1]: https://github.com/pybricks/pybricks-micropython/compare/v3.1.0...v3.2.0b1
[3.1.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0c1...v3.1.0
[3.1.0c1]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a4...v3.1.0c1
[3.1.0b1]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a4...v3.1.0b1
[3.1.0a4]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a3...v3.1.0a4
[3.1.0a3]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a2...v3.1.0a3
[3.1.0a2]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0a1...v3.1.0a2
[3.1.0a1]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0...v3.1.0a1
[3.0.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.0.0c1...v3.0.0
