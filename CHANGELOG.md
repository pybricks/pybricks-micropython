<!-- Refer to https://keepachangelog.com/en/1.0.0/ for guidance. -->

# Changelog

## [Unreleased]

### Added

- Allow Bluetooth to be toggled off and on with the Bluetooth button on the
  Prime Hub and the Inventor Hub ([support#1615]), and have this state persist
  between reboots.
- Added `heading_correction` to `hub.imu.settings` to allow for automatic
  correction of the `hub.imu.heading()` value ([support#1678]).
- Added `update_heading_correction` to interactively set the heading
  correction value ([support#1678]).

### Changed

- When upgrading the firmware to a new version, the user program will now
  be erased. This avoids issues with incompatible program files ([support#1622]).
- The `angular_velocity_threshold`, and `acceleration_threshold` settings
  in `hub.imu.settings` are now persistent between reboots.
- Reduced hub poweroff time from 3 to 2 second to make it easier to turn off
  the hub ([pybricks-micropython#250]).
- Improved font for the digits ``0--9`` when displaying them
  with `hub.display.char(str(x))` ([pybricks-micropython#253]).

### Fixed
- Fixed not able to connect to new Technic Move hub with `LWP3Device()`.

[pybricks-micropython#250]: https://github.com/pybricks/pybricks-micropython/pull/250
[pybricks-micropython#253]: https://github.com/pybricks/pybricks-micropython/pull/253
[support#1615]: https://github.com/pybricks/support/issues/1615
[support#1622]: https://github.com/pybricks/support/issues/1622
[support#1678]: https://github.com/pybricks/support/issues/1678

## [3.5.0] - 2024-04-11

### Changed
- Bump version from release candidate to 3.5.0 without additional changes.

## [3.5.0b2] - 2024-04-05

### Added
- Added optional keyword arguments to `pybricks.tools.read_input_byte()` for
  automatic conversion via `chr` and to skip to the last byte ([support#1574]).
- Added `disconnect` method to `pybricks.pupdevices.Remote` and
  `pybricks.iodevices.LWP3Device` ([support#802]).

### Changed
- Raise a descriptive error when the `Car` class can't find a steering mechanism
  end stop within 10 seconds ([support#1564]).
- Extended region of readable data with `hub.system.storage` to include
  user program ([pybricks-micropython#243]).

### Fixed
- Fixed hubs not shutting down when holding hub button ([support#1419]).

[pybricks-micropython#243]: https://github.com/pybricks/pybricks-micropython/pull/243
[support#802]: https://github.com/pybricks/support/issues/802
[support#1419]: https://github.com/pybricks/support/issues/1419
[support#1564]: https://github.com/pybricks/support/issues/1564
[support#1574]: https://github.com/pybricks/support/issues/1574

## [3.5.0b1] - 2024-03-21

### Added
- Added support for rumble in `XboxController` ([support#1024]).
- Added `Button.UP`, `Button.DOWN`, `Button.LEFT`, and `Button.RIGHT` to
  `XboxController` buttons method ([support#1537]). The separate directional
  pad method remains available.

### Changed
- Allow single floating point value for brightness array ([support#1547]).

[support#1024]: https://github.com/pybricks/support/issues/1024
[support#1537]: https://github.com/orgs/pybricks/discussions/1537
[support#1547]: https://github.com/pybricks/support/issues/1547

## [3.4.0] - 2024-03-11

### Changed
- Additional error checking in `pybricks.robotics.Car` setup ([support#1502]).

[support#1502]: https://github.com/pybricks/support/issues/1502

## [3.4.0b3] - 2024-03-05

### Added
- Added `joystick_deadzone` keyword argument to suppress controller drift in
  the `XboxController` ([support#1473]).
- Added `pybricks.tools.running` to return if a stopwatch is currently running
  ([support#1490]).

### Changed
- Use `Button` parameter for `XboxController` ([support#1488]), not strings.
- If `pybricks.tools.run_task` is called without arguments, it will return
  whether the runloop is running or not ([support#1499]).
- On Move Hub, the verbosity of error messages is further reduced to reduce
  the firmware size, to allow for bug fixes/updates ([pybricks-micropython#240]).
- Re-implemented `pybricks.robotics.Car` in C so it can be enabled on Move Hub
  which does not support frozen modules ([support#1502]).

### Fixes
- Fix `pybricks.iodevices` not allowing writing -128 value ([support#1366]) and
  raise informative error messages instead of clamping the input.

[pybricks-micropython#240]: https://github.com/pybricks/pybricks-micropython/pull/240
[support#1366]: https://github.com/pybricks/support/issues/1366
[support#1453]: https://github.com/pybricks/support/issues/1453
[support#1473]: https://github.com/pybricks/support/issues/1473
[support#1488]: https://github.com/pybricks/support/issues/1488
[support#1490]: https://github.com/pybricks/support/issues/1490
[support#1499]: https://github.com/pybricks/support/issues/1499
[support#1502]: https://github.com/pybricks/support/issues/1502

## [3.4.0b2] - 2024-02-10

### Added
- Added `pybricks.iodevices.XboxController` ([support#191], [support#1024]).
- Re-enable `pybricks.iodevices.LWP3Device` missing from last beta.

### Fixes
- Fix hub not shutting down while a program runs ([support#1438]).

### Changed
- On Technic Hub and City Hub, the Bluetooth chip is now configured to reject
  connection parameter updates from peripherals. This avoids interference with
  broadcast when used with the Xbox controller.

[support#191]: https://github.com/pybricks/support/issues/191
[support#1024]: https://github.com/pybricks/support/issues/1024
[support#1438]: https://github.com/pybricks/support/issues/1438

## [3.4.0b1] - 2024-01-30

### Added
- Added `pybricks.robotics.Car` for controlling a car with one or more drive
  motors and a steering motor. This is a convenience class that combines
  several motors to provide the functionality used in most Technic cars.

### Fixes
- Fix observing stopping on City and Technic hubs after some time ([support#1096]).
- Fix Bluetooth locking up when connecting Bluetooth adapter with small MTU to Technic and City hubs ([support#947]).
- Fix Technic Hub not always starting automatically after firmware exiting
  update mode ([support#1408]). Also apply this to Move Hub and City Hub.
- Fix Bluetooth locking up when connecting Bluetooth adapter with small MTU to
  Technic and City hubs ([support#947]).
- Fix Remote pairing difficulty ([support#880]).
- Fix Remote light not working or crashing the hub ([support#1357]).
- Fix Technic Hub and City Hub broadcasting missing messages ([support#1357]).
- Fix Technic Hub and City Hub broadcasting delays.

### Changed
- Changed polarity of output in the `Light` class. This makes no difference for
  the Light class, but it makes the class usable for certain custom
  devices ([pybricks-micropython#166]).
- Improved Bluetooth peripheral scanning and connect process to allow for new
  device types to be added in the future.

### Changed
- Changed keypad return type to set instead of tuple. This affects the remote
  and hubs.

[support#880]: https://github.com/pybricks/support/issues/880
[support#947]: https://github.com/pybricks/support/issues/947
[support#1096]: https://github.com/pybricks/support/issues/1096
[support#1357]: https://github.com/pybricks/support/issues/1357
[support#1408]: https://github.com/pybricks/support/issues/1408
[pybricks-micropython#222]: https://github.com/pybricks/pybricks-micropython/pull/222

## [3.3.0] - 2023-11-24

### Changed
- Bump version from release candidate to 3.3.0.

## [3.3.0c1] - 2023-11-20

### Added
- Added `MoveHub.imu.tilt()` ([support#539]).
- Enabled hub init orientation support for Move Hub ([support#539]).

### Changed
- Allow Move Hub to ignore `broadcast` instead of raising an exception while
  connected.

### Fixed
- Fixed Move Hub accelerometer not working since v3.3.0b5 ([support#1269]).
- Fixed Bluetooth chip locking up on Technic and City hubs when broadcasting ([support#1095]).
- Fixed potential crash when GC occurs while observing BLE data ([support#1278])
- Fixed Technic Hub and City Hub eventually stopping observing BLE data after
  a few minutes ([support#1096]) by implementing an auto-reset workaround.

[support#539]: https://github.com/pybricks/support/issues/539
[support#1095]: https://github.com/pybricks/support/issues/1095
[support#1096]: https://github.com/pybricks/support/issues/1096
[support#1269]: https://github.com/pybricks/support/issues/1269
[support#1278]: https://github.com/pybricks/support/issues/1278

## [3.3.0b9] - 2023-10-26

### Added
- Added `hub.buttons` as an alias for `hub.button` on buttons with one
  hub ([support#1254]).
- Implemented `brake` for `DriveBase` ([support#881]).

### Changed
- The `use_gyro` method is added to the normal `DriveBase` class instead of
  having a separate `GyroDriveBase` class. Since the latter was only released
  in beta versions, this is not a breaking change ([support#1054]).
- New color distance function used by the color sensors that is more
  consistent when distinguishing user-provided colors ([pybricks-micropython#104]).
- Updated the unreleased BLE API to ensure sent and received objects are the
  same. Allows one of the supported types or a list/tuple thereof.

### Fixed
- Improved external device detection speed ([support#1140]).
- Fixed Powered Up Tilt Sensor not working  ([support#1189]).
- Fixed `surface=False` not working in `ColorSensor` ([support#1232]).
- Fixed `PUPDevice.write` not selecting correct mode ([support#1213]).
- Fixed City Hub turning back on after shutdown ([support#1195]).
- Fixed SPIKE hubs not broadcasting at all when attempting to broadcast in a
  tight loop ([support#1151]).

[pybricks-micropython#104]: https://github.com/pybricks/pybricks-micropython/pull/104
[support#881]: https://github.com/pybricks/support/issues/881
[support#1054]: https://github.com/pybricks/support/issues/1054
[support#1140]: https://github.com/pybricks/support/issues/1140
[support#1151]: https://github.com/pybricks/support/issues/1151
[support#1189]: https://github.com/pybricks/support/issues/1189
[support#1195]: https://github.com/pybricks/support/issues/1195
[support#1213]:  https://github.com/pybricks/support/issues/1213
[support#1232]: https://github.com/pybricks/support/issues/1232
[support#1254]: https://github.com/pybricks/support/issues/1254

## [3.3.0b8] - 2023-07-07

### Added
- Added `use_gyro` method to toggle gyro use on and off in the `GyroDriveBase`
  class ([support#1054]).
- Added `pybricks.tools.read_input_byte()` function ([support#1102]).

### Changed
- Relaxed thresholds that define when the IMU is stationary, to make the
  defaults work better in noisier conditions ([support#1105]).

### Fixed
- Fixed Technic (Extra) Large motors not working ([support#1131]) on all hubs.
- Fixed Powered Up Light not working ([support#1131]) on all hubs.
- Fixed UART sensors not working on Technic Hub ([support#1137]).
- Fixed incorrect number of ports on City Hub ([support#1131]).

[support#1054]: https://github.com/pybricks/support/issues/1054
[support#1105]: https://github.com/pybricks/support/issues/1105
[support#1131]: https://github.com/pybricks/support/issues/1131
[support#1137]: https://github.com/pybricks/support/issues/1137


## [3.3.0b7] - 2023-06-30

### Added
- Added `'modes'` entry to the dictionary returned by `PUPDevice.info()`. It
  is a tuple of `(name, num_values, data_type)` tuples for each available mode.
- Added `pybricks.tools.read_input_byte()` function ([support#1102]).
- Added `pybricks.tools.hub_menu()` function ([support#1064]).

### Changed
- Changed internal drivers for LEGO devices (motors and sensors) on all platforms.

### Fixed
- Fixed hub will not power off when Bluetooth chip crashes on City and Technic hubs ([support#1095]).
- Fixed `off()` method in `ColorLightMatrix`, `UltrasonicSensor`, `ColorSensor` ([support#1098]).

[support#1064]: https://github.com/pybricks/support/issues/1064
[support#1095]: https://github.com/pybricks/support/issues/1095
[support#1098]: https://github.com/pybricks/support/issues/1098
[support#1102]: https://github.com/pybricks/support/issues/1102

## [3.3.0b6] - 2023-06-02

### Added
- Enabled builtin `set` type (except on BOOST Move hub) ([support#402]).
- Added experimental support for multitasking ([pybricks-micropython#166]).

### Changed
- Updated BTStack to v1.5.5.

### Fixed
- Fixed BLE broadcast not working on City hub.
- Fixed crash on BTStack hubs when program stopped during call to `ble.broadcast()`.
- Fixed BLE broadcast not working on Technic hub when not connected ([support#1086]).
- Fixed delayed sensor sync on boot on City hub ([support#747]).

[pybricks-micropython#166]: https://github.com/pybricks/pybricks-micropython/pull/166
[support#402]: https://github.com/pybricks/support/issues/402
[support#747]: https://github.com/pybricks/support/issues/747
[support#1086]: https://github.com/pybricks/support/issues/1086

## [3.3.0b5] - 2023-05-16

### Added
- Enabled the `gc` module (except on BOOST Move hub).
- Added `hub.ble` attribute for broadcasting/observing ([pybricks-micropython#158]).

### Changed
- Updated MicroPython to v1.20.0.

### Fixed
- Fixed stdin containing `0x06` command byte ([support#1052]).
- Fixed motor process causing delays on ev3dev ([support#1035]).

[pybricks-micropython#158]: https://github.com/pybricks/pybricks-micropython/pull/158
[support#1035]: https://github.com/pybricks/support/issues/1035
[support#1052]: https://github.com/pybricks/support/issues/1052

## [3.3.0b4] - 2023-04-21

### Fixed
- Fixed gyro on Technic Hub occasionally giving a bad value, which made it
  not calibrate properly ([support#1026]).
- Fixed discrepancy in heading value across hubs by accounting for sampling
  time ([support#1022]).
- Fixed iterator for `Matrix` objects giving bad values.
- Fixed Bluetooth sometimes locking up on Technic/City hubs ([support#567]).
- Fixed `GyroDriveBase` being slow to respond to heading perturbations when
  driving at high speed ([support#1032]).

### Added
- Added `pybricks.tools.cross(a, b)` to get a vector cross product.
- Added experimental implementation of `hub.imu.heading()` ([support#912]).
- Added support for reading single-axis rotation, which is useful in
  applications like balancing robots, where full 3D orientation is not
  required, or even undesired.
- Added `hub.imu.ready()` to check that the IMU has been calibrated and is
  ready for use.
- Added `GyroDriveBase` class to control drivebase steering with the gyro.
- Added optional `window` parameter to `Motor.speed` to specify the
  differentiation window size that determines the average speed. This lets the
  user choose smaller values to get a more responsive (but noisier) or higher
  values to get a smoother (but more delayed) speed signal.

### Removed
- Removed `positive_direction` from `DriveBase` initializer. This was
  temporarily added in the previous beta release to facilitate gyro support,
  but made it more complicated than needed ([support#992]).
- Removed `pybricks.geometry` in an effort to reduce the number of modules with
  just a few elements. `Matrix` and `vector` have moved to `tools`. The `Axis`
  enum was moved to `parameters`. Each item can still be imported from its
  original location for backwards compatibility.

[support#567]: https://github.com/pybricks/support/issues/567
[support#992]: https://github.com/pybricks/support/issues/992
[support#1022]: https://github.com/pybricks/support/issues/1022
[support#1026]: https://github.com/pybricks/support/issues/1026
[support#1032]: https://github.com/pybricks/support/issues/1032

## [3.3.0b3] - 2023-03-28

### Added
- Added `positive_direction` to `DriveBase` initializer. It defaults to
  clockwise to ensure this is not a breaking change. Users can now change it
  to counterclockwise, which is more common in engineering ([support#989]).
- Added support for setting drivebase acceleration and deceleration separately
  using a tuple, consistent with single motors ([support#881]).

### Fixed
- Fixed allocator interfering with motor control when memory usage is high ([support#977]).
- Fixed `Stop.NONE` not working properly for some drivebase geometries ([support#972]).
- Fixed reading programs larger than 65535 bytes on boot on SPIKE hubs. ([[support#996]).
- Various Bluetooth stability and reliability improvements on BOOST Move hub
  ([support#320], [support#324], [support#417]).
- Fixed Bluetooth random address not changing on City and Technic hubs ([support#1011]).

### Changed
- Methods like `control.limits()` now check the user input and raise a
  `ValueError` if a value is out of bounds  ([support#484]). This affects only
  settings setters, which are usually used as a one-off. Nothing changes to
  speed values set at runtime. These are still capped to valid numbers without
  raising exceptions.
- Renamed `precision_profile` to `profile` in the `Motor` initializer.
- In `DriveBase`, `wheel_diameter` and `axle_track` now accept decimal values
  for increased precision ([support#830]).

### Removed
- Removed `DriveBase.left` and `DriveBase.right` properties ([support#910]).

[support#320]: https://github.com/pybricks/support/issues/320
[support#324]: https://github.com/pybricks/support/issues/324
[support#417]: https://github.com/pybricks/support/issues/417
[support#484]: https://github.com/pybricks/support/issues/484
[support#830]: https://github.com/pybricks/support/issues/830
[support#881]: https://github.com/pybricks/support/issues/881
[support#910]: https://github.com/pybricks/support/issues/910
[support#912]: https://github.com/pybricks/support/issues/912
[support#972]: https://github.com/pybricks/support/issues/972
[support#977]: https://github.com/pybricks/support/issues/977
[support#989]: https://github.com/pybricks/support/issues/989
[support#996]: https://github.com/pybricks/support/issues/996
[support#1011]: https://github.com/pybricks/support/issues/1011

## [3.3.0b2] - 2023-03-08

### Added
- Added `precision_profile` parameter to `Motor` initializer. This can be used
  to reduce control gains to get smoother motions for heavy loads or heavily
  gear applications where precision is less relevant.

### Changed
- Changed how the PID values are initialized for each motor. This may lead to
  slightly altered performance.

### Fixed
- Fixed move hub crashing on boot.
- Fixed position based commands starting from the wrong position if the
  previous command was a time based command that could not hit its
  target ([support#956]).
- Fixed EV3 motors getting out of date with the updated motor
  controllers ([support#941]) and ([support#955]).
- Fixed long delay when connecting to remote on SPIKE hubs ([support#466]).

[support#466]: https://github.com/pybricks/support/issues/466
[support#941]: https://github.com/pybricks/support/issues/941
[support#955]: https://github.com/pybricks/support/issues/955
[support#956]: https://github.com/pybricks/support/issues/956

## [3.3.0b1] - 2023-02-17

### Added
- Added support for frozen modules when building from source ([support#829]).
- Added `close()` method to `DCMotor` and `Motor` so they can be closed and
  re-initialized later ([support#904]).
- Fixed workaround for motor hold drifting away under external input
  movement ([support#863]).
- Added `Motor.model` object to interact with the motor state estimator.
- Added `Stop.BRAKE_SMART` as `then` option for motors. It works just like
  `SMART_COAST`, but with passive electrical braking.
- Added logging support for control stall and pause state.

### Fixed
- Fixed `Light` controlling wrong ports on Move hub ([support#913]).
- Reduced motor motion while holding position and added configurable setter and
  getter for this deadzone.
- Fixed type checking optimized out on Move hub ([support#950]).
- Fixed end-user stall flag coming up too early in position based control.
- Further reduced stutter at low motor speeds  ([support#366]).

[support#366]: https://github.com/pybricks/support/issues/366
[support#829]: https://github.com/pybricks/support/issues/829
[support#863]: https://github.com/pybricks/support/issues/863
[support#904]: https://github.com/pybricks/support/issues/904
[support#913]: https://github.com/pybricks/support/issues/913
[support#950]: https://github.com/pybricks/support/issues/950

## [3.2.2] - 2023-01-06

### Fixed
- Fixed some objects do not implement `__hash__` ([support#876]).
- Fixed `Motor.run_time` not completing under load ([support#903]).

[support#876]: https://github.com/pybricks/support/issues/876
[support#903]: https://github.com/pybricks/support/issues/903

## [3.2.1] - 2022-12-26

### Fixed
- Fixed `imu.angular_velocity` returning the values of `imu.acceleration`.

[support#885]: https://github.com/pybricks/support/issues/885

## [3.2.0] - 2022-12-20

### Changed
- Buffered stdout is flushed before ending user program.

### Fixed
- Fixed SPIKE/MINDSTORMS hubs advertising after disconnect while user program
  is still running ([support#849]).
- Fixed Essential hub hanging on boot when bootloader entered but USB cable
  not connected ([support#821]).
- Fixed button needs debouncing on City/Technic/Essential hubs ([support#716]).
- Fixed motor hold drifting away under external input movement ([support#863]).

[support#716]: https://github.com/pybricks/support/issues/716
[support#821]: https://github.com/pybricks/support/issues/821
[support#849]: https://github.com/pybricks/support/issues/849
[support#863]: https://github.com/pybricks/support/issues/863

## [3.2.0c1] - 2022-12-09

### Fixed
- Fixed `motor.control.limits()` not working if acceleration was `None`.
- Fixed crash on calling methods on uninitialized objects ([support#805]).
- Fixed crash on calling methods in `__init__(self, ...)` before
  calling `super().__init(...)` on uninitialized objects ([support#777]).
- Reverted Pybricks Code stop button raises `SystemAbort` instead of
  `SystemExit` ([support#834]).
- Improved stop message raised on `SystemExit` and `SystemAbort` ([support#836]).
- Fixed Technic Hub and City Hub sometimes not shutting down when a Bluetooth
  operation is busy ([support#814]).
- Fixed `hub.system` methods not working ([support#837]).

### Changed
- Changed default XYZ orientation of the Technic Hub and the Essential Hub to
  match the SPIKE Prime Hub and Move Hub ([support#848]).

[support#777]: https://github.com/pybricks/support/issues/777
[support#805]: https://github.com/pybricks/support/issues/805
[support#814]: https://github.com/pybricks/support/issues/814
[support#826]: https://github.com/pybricks/support/issues/826
[support#834]: https://github.com/pybricks/support/issues/834
[support#836]: https://github.com/pybricks/support/issues/836
[support#837]: https://github.com/pybricks/support/issues/837
[support#848]: https://github.com/pybricks/support/issues/848


## [3.2.0b6] - 2022-12-02

### Added
- Added support for `PBIO_PYBRICKS_COMMAND_REBOOT_TO_UPDATE_MODE` Pybricks
  Profile BLE command.
- Implemented `Motor.load()` which now measures load both during active
  conditions (`run`) and passive conditions (`dc`).

### Changed
- The Pybricks Code stop button will force the program to exit even if the user
  catches the `SystemExit` exception ([pybricks-micropython#117]).
- Changed `PrimeHub.display.image()` to `PrimeHub.display.icon()` and renamed
  its kwarg from `image` to `icon` ([support#409]).
- Deprecated `Control.load()`, `Control.stalled()`, and `Control.done()`
  methods, but they will continue to exist in the firmware until further
  notice ([support#822]). New scripts are encouraged to use the (improved)
  variants available directly on `Motor` objects.

### Fixed
- Fixed connecting `Remote` on BOOST move hub ([support#793]).

### Removed
- Removed `hub.system.reset()` method.
- Disabled `micropython` module on Move Hub.

[pybricks-micropython#117]: https://github.com/pybricks/pybricks-micropython/pull/117
[support#409]: https://github.com/pybricks/support/issues/409
[support#793]: https://github.com/pybricks/support/issues/793
[support#822]: https://github.com/pybricks/support/issues/822

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
[Unreleased]: https://github.com/pybricks/pybricks-micropython/compare/v3.5.0...HEAD
[3.5.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.5.0b2...v3.5.0
[3.5.0b2]: https://github.com/pybricks/pybricks-micropython/compare/v3.5.0b1...v3.5.0b2
[3.5.0b1]: https://github.com/pybricks/pybricks-micropython/compare/v3.4.0...v3.5.0b1
[3.4.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.4.0b3...v3.4.0
[3.4.0b3]: https://github.com/pybricks/pybricks-micropython/compare/v3.4.0b2...v3.4.0b3
[3.4.0b2]: https://github.com/pybricks/pybricks-micropython/compare/v3.4.0b1...v3.4.0b2
[3.4.0b1]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0...v3.4.0b1
[3.3.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0c1...v3.3.0
[3.3.0c1]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b9...v3.3.0c1
[3.3.0b9]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b8...v3.3.0b9
[3.3.0b8]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b7...v3.3.0b8
[3.3.0b7]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b6...v3.3.0b7
[3.3.0b6]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b5...v3.3.0b6
[3.3.0b5]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b4...v3.3.0b5
[3.3.0b4]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b3...v3.3.0b4
[3.3.0b3]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b2...v3.3.0b3
[3.3.0b2]: https://github.com/pybricks/pybricks-micropython/compare/v3.3.0b1...v3.3.0b2
[3.3.0b1]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.2...v3.3.0b1
[3.2.2]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.1...v3.2.2
[3.2.1]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0...v3.2.1
[3.2.0]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0c1...v3.2.0
[3.2.0c1]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0b6...v3.2.0c1
[3.2.0b6]: https://github.com/pybricks/pybricks-micropython/compare/v3.2.0b5...v3.2.0b6
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
