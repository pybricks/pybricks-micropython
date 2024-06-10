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

## Requirements

Since USB communication is not yet implemented, you need the following hardware
to use this:

- A USB serial adapter to connect to sensor port 1 on the EV3 brick. You could
use [this one](http://www.mindsensors.com/ev3-and-nxt/40-console-adapter-for-ev3).
Or make your own using a standard USB to serial adapter and connecting it to the
data wires of an EV3 cable.
- A microSD card (32 GB or less) to store the firmware image.

## Prerequisites

Before attempting to build this, please follow the instructions to build the
firmware for one of the other targets, such as the SPIKE Prime Hub, as
explained [here](../../CONTRIBUTING.md).

Then, install the following additional tools:

```
sudo apt install u-boot-tools
```

Prepare a microSD card (32 GB or less) and format it as a single FAT32
partition. It is not necessary needed to set any boot flags.

In the following examples, we assume that the formatted volume is called
`ev3`.

## Building the firmware


```bash
# Navigate to the repository.
cd pybricks-micropython

# Build the uImage.
make -C bricks/ev3 clean
make -C bricks/ev3 uImage -j

# Copy the result and resources to the root of microSD card, e.g:
cp bricks/ev3/build/uImage /media/user_name/ev3/uImage

```

Note: This should not be confused with other existing or outdated EV3 builds in the `bricks` folder such as the `ev3dev` or `ev3rt` builds. They can serve as inspiration, but are completely separate from this build. From a code point of view, this new `bricks/ev3` build will be a lot more like `bricks/primehub`.

## Operating the brick

- Connect your serial adapter to sensor port 1 on the EV3 brick.
- Start a terminal emulator such as `screen` or `picocom`.
- Insert the microSD card into the EV3 brick.
- Press the center button to boot.

You should see something like the following output:

```
EV3 initialization passed!
Booting EV3 EEprom Boot Loader

	EEprom Version:   0.60
	EV3 Flashtype:    N25Q128A13B

EV3 Booting system 

Jumping to entry point at: 0xC1080000


U-Boot 2009.11 (Oct 26 2012 - 10:30:38)

I2C:   ready
DRAM:  64 MB
MMC:   davinci: 0
In:    serial
Out:   serial
Err:   serial
ARM Clock : 300000000 Hz
DDR Clock : 132000000 Hz
Invalid MAC address read.
Hit 'l' to stop autoboot:  0 
reading boot.scr

** Unable to read "boot.scr" from mmc 0:1 **
reading uImage

209016 bytes read
## Booting kernel from Legacy Image at c0007fc0 ...
   Image Name:   
   Image Type:   ARM Linux Kernel Image (uncompressed)
   Data Size:    208952 Bytes = 204.1 kB
   Load Address: c0008000
   Entry Point:  c0008000
   Loading Kernel Image ... OK
OK

Starting kernel ...

System init in platform.c called from startup.s

Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Hello, world at time (ms): 0
Traceback (most recent call last):

  File "%q", line %dhello.py", line %d9���D�, in %q
<module>
KeyboardInterrupt:
Pybricks MicroPython v1.20.0-23-g6c633a8dd on 2024-06-08; MINDSTORMS EV3 Brick with TI Sitara AM1808
Type "help()" for more information.
>>> 
```

You can remove the microSD card after booting. After updating the uImage, you
can try out your new build by rebooting: Press and hold the center and back
buttons for 4 seconds.

For now, there is just the REPL and several builtin MicroPython modules.
Sensors and motors are not yet enabled.

## Development status

This is a highly experimental development. Pretty much nothing is enabled yet.
The intention is to prepare a minimal build where we can add drivers one by
one, with help from experts in the community.

Inspiration for future hardware implementation:
- [ev3dev](https://www.ev3dev.org/docs/kernel-hackers-notebook/ev3dev-linux-kernel/): Well documented resources for EV3 on debian Linux.
- [EV3RT](https://github.com/pybricks/ev3rt-lib): RTOS (not Linux). Closer to the metal, but drivers mixed in with RTOS.
- [am18xlib](https://github.com/pybricks/am18x-lib-ev3): Good inspiration for minimal peripheral drivers, including PRU and so on.
