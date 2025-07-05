# Pybricks on EV3 without Linux (Work in progress)

This Pybricks port is intended to run on EV3 as a "bare metal" port, _without_
an underlying operating system such as `ev3dev` or `TOPPERS/EV3RT`.

The goal is to make it very similar to the other "bare metal" ports such as the
ones we have for the SPIKE Prime Hub, Technic Hub, City Hub, and the BOOST Move
Hub. By making it work the same, it will become part of the same family of
hubs. It will be able to use the same online code editor, so that EV3 can be
used well into the future, even when the official apps are all discontinued.

This should resolve many open requests to continue support for EV3 in Pybricks
3.X. It should also resolve technical issues related to device detection and
[loop time issues](https://github.com/pybricks/support/issues/1035).

Since it won't have Linux, it won't have certain features like Wi-Fi or support
for advanced accessories. For those uses cases, Pybricks 2.0 on ev3dev will
remain available.

## Prerequisites

Before attempting to build this, please follow the instructions to build the
firmware for one of the other targets, such as the SPIKE Prime Hub, as
explained [here](../../CONTRIBUTING.md). Make sure that `pybricksdev` is
installed.

Unlike most other alternative EV3 firmware solutions, Pybricks does not require
using a microSD card. Instead, Pybricks is installed as a firmware update.


## Put EV3 into update mode

- Attach the USB cable (you can leave it plugged in all the time).
- Make sure the EV3 is off.
- Hold right button and start the EV3 with the center button.
- The display should now say "Updating.."

## Building and deploying the firmware

```bash
# Navigate to the repository.
cd pybricks-micropython

# Optional: clean.
make -C bricks/ev3 clean

# Build firmware and deploy with Pybricksdev.
make -C bricks/ev3 -j deploy
```

Instead, you can download the [latest nightly build](https://nightly.link/pybricks/pybricks-micropython/workflows/build/master). Install it as follows:

```
pybricksdev flash ~/Downloads/ev3-firmware-build-3782-git1bcea603.zip
```

## Operating the brick

Press the center button to turn on the EV3. Press the back button to turn it
off.

If the hub freezes, press and hold the back and center buttons for several seconds to
reboot.

## Interfacing with the EV3

The data wires on sensor port 1 are set up as a UART for debugging. This
provides the MicroPython REPL. All other ports can be used normally.

A REPL is also available as a USB serial device
after you reboot the brick at least once. This feature may go away when we enable a proper USB driver for downloading and running programs.

## Development status

This is a highly experimental development. Sensors, motors, and some EV3 peripherals [are working](https://www.youtube.com/watch?v=9Iu6YpFLwKo). Please refer to our discussion forums for a status or to help with ongoing developments.

Inspiration for future hardware implementation:
- [ev3dev](https://www.ev3dev.org/docs/kernel-hackers-notebook/ev3dev-linux-kernel/): Well documented resources for EV3 on debian Linux.
- [EV3RT](https://github.com/pybricks/ev3rt-lib): RTOS (not Linux). Closer to the metal, but drivers mixed in with RTOS.
- [am18xlib](https://github.com/pybricks/am18x-lib-ev3): Good inspiration for minimal peripheral drivers, including PRU and so on.
