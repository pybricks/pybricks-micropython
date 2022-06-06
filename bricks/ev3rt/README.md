# Pybricks on EV3RT

This Pybricks port runs on [EV3RT](https://github.com/pybricks/ev3rt-lib), an
RTOS for Mindstorms EV3 with the TOPPERS/HRP3 Kernel.

## Prerequisites

Before attempting to build this, please follow the instructions to build the
firmware for one of the other targets, such as the SPIKE Prime Hub.

Then, install the following additional tools:

```
sudo apt install ruby u-boot-tools
```

Prepare a microSD card (16 GB or less) and format it as a single FAT32
partition. It is not necessary needed to set any boot flags.

In the following examples, we assume that the formatted volume is called
`EV3RT`.

## Building the firmware


```bash
# Navigate to the repository.
cd pybricks-micropython

# Build the uImage.
make ev3rt

# Copy the result and resources to the root of microSD card, e.g:
cp bricks/ev3rt/build/uImage /media/user_name/EV3RT/uImage
cp bricks/ev3rt/pybricks.bmp /media/user_name/EV3RT/pybricks.bmp

```

## Operating the brick

Insert the microSD card into the EV3 brick and press the center button to boot.
After a few seconds, you should see the [Pybricks logo](pybricks.bmp) on the
display.

Press and hold the back button for 3 seconds to return to the EV3RT console
menu. From there, press the right button and then the center button to select
`Shutdown` and power off the hub.

## Accessing MicroPython

When the brick is powered on, bluetooth is automatically enabled. To connect,
search for the brick in your computer's bluetooth settings, and pair.

The pairing pin is `0000`.

Once paired, your system should set up a comport that you can connect to with
your favorite terminal emulator. Then you can use the MicroPython REPL.

## Ubuntu 22.04 connection instructions

For Ubuntu users, it is recommended to use the Bluetooth Manager to search for
the brick and connect to it. You also need to be a member of the `dialout`
group:

```
sudo apt install blueman screen
sudo adduser $USER dialout
```

You may need to log out or reboot for this setting to become effective.

Open the Bluetooth Manager from your app menu. If you cannot find the brick via
`Search`, follow the next steps once:


```
bluetoothctl
```
In the console that appears, type:
```
scan on
```
Wait for the device to show up. If you're not sure which device is your EV3,
check the Bluetooth address on the EV3RT console menu on the screen (see
instructions above).

Type `pair`, followed by the address, e.g.:

```
pair A0:E6:F8:2C:5B:4D
```
Enter the pin when prompted to do so. After doing this once, the device should
show up in the Bluetooth Manager. 

In the Bluetooth Manager, right-click on the device and choose `Connect to
Serial Port`. Once, connected, right-click the device again to see which port
it was connected to. This should be something like `rfcomm0`.

Now you can access the MicroPython REPL using:

```
screen /dev/rfcomm0 115200
```

## Development tricks

If you connect the EV3 to your computer with a USB cable, it will automatically
mount the microSD card for easy file access.

You must unmount/eject the volume from your computer before you can turn the
brick off, but this can be automated. For example, if the device `sdb1` always
gets mounted at `/media/$USER/EV3RT/`, the build and deployment can be done with
the following one-liner:

```
make ev3rt -j && cp bricks/ev3rt/build/uImage /media/$USER/EV3RT/uImage && umount /dev/sdb1 && udisksctl power-off --block-device /dev/sdb1
```


## Development status

For now, there is just the REPL and several builtin MicroPython modules.
Sensors and motors are not yet enabled.
