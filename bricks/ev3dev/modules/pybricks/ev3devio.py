# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2019 Laurens Valk

"""Read and write to EV3 sensors through ev3dev sysfs."""
from os import listdir, path
from .tools import wait, StopWatch


def read_int(infile):
    """Read an integer from a previously opened file descriptor."""
    infile.seek(0)
    return(int(infile.read().decode().strip()))


def write_int(outfile, value):
    """Write an integer to a previously opened file descriptor."""
    outfile.write(str(int(value)))
    outfile.flush()


def read_str(infile):
    """Read a string from a previously opened file descriptor."""
    infile.seek(0)
    return(infile.read().decode().strip())


def write_str(outfile, value):
    """Write a string to a previously opened file descriptor."""
    outfile.write(value)
    outfile.flush()


def get_sensor_path(port, driver_name):
    """Get a path to a device based on port number."""
    base_dir = '/sys/class/lego-sensor'
    # Iterate through ['sensor0', 'sensor1', 'sensor2', ...]
    for device_dir in listdir(base_dir):
        # In each folder, open the address file
        with open(base_dir + '/' + device_dir + '/address', 'r') as addr_file:
            # Read the port string (e.g. 'outB')
            port_found = addr_file.read().strip('\n')
            # If the port name matches, we are done searching and
            # we know the full path
            if 'in' + chr(port) in port_found:
                # Make the full path
                full_dir = base_dir + '/' + device_dir + '/'
                with open(full_dir + 'driver_name', 'r') as driver_file:
                    if driver_name in driver_file.read():
                        return full_dir
    raise OSError('No such sensor on Port S' + chr(port))


class Ev3devSensor():
    """Base class for ev3dev sensors operating through sysfs."""

    _ev3dev_driver_name = 'none'
    _number_of_values = 1
    _default_mode = None

    def __init__(self, port):
        """Initialize the sensor."""
        assert ord('1') <= port <= ord('4')
        self.port = port
        self._open_files()
        if self._default_mode:
            self._mode(self._default_mode)

    def _open_files(self):
        """Open the sysfs files for this device."""
        self.path = get_sensor_path(self.port, self._ev3dev_driver_name)
        self.mode_file = open(self.path + 'mode', 'r+b')
        self.mode_now = read_str(self.mode_file)
        self.value_files = [open(self.path + 'value' + str(num), 'rb') for num in range(self._number_of_values)]

    def _close_files(self):
        """Close the sysfs files for this device."""
        self.mode_file.close()
        for value_file in self.value_files:
            value_file.close()

    def _mode(self, mode_new):
        """Set the sensor mode. Not available to user."""
        if mode_new != self.mode_now:
            write_str(self.mode_file, mode_new)
            self.mode_now = mode_new

    def _value(self, num):
        """Return value in sensor/valueX. Not available to user."""
        return read_int(self.value_files[num])


class Ev3devUartSensor(Ev3devSensor):
    """UART ev3dev sensor operating through sysfs."""

    def _reset_port(self):
        path = '/sys/class/lego-port/port' + chr(self.port-1) + '/'
        with open(path + 'mode', 'w') as rf:
            rf.write('auto')
        with open(path + 'mode', 'w') as rf:
            rf.write('ev3-uart')
        watch = StopWatch()
        while True:
            with open(path + 'status', 'r') as sf:
                status = sf.read().strip()
                if status != "no-sensor":
                    break
            wait(100)
            if watch.time() > 5000:
                raise OSError("Unable to reset sensor.")

    def _reset(self):
        """Force sensor to reset as if disconnecting and reconnecting it."""
        # First, close all files for this sensor
        try:
            self._close_files()
        except ValueError as err:
            pass
        # Reset the sensor
        self._reset_port()
        total = StopWatch()
        sub = StopWatch()
        # Wait for sensor to come back
        wait(2000)
        success = False
        while total.time() < 15000:
            wait(250)
            if sub.time() > 5000:
                self._reset_port()
                sub.reset()
            try:
                self._open_files()
                success = True
            except OSError:
                continue
            break

        if not success:
            raise OSError("Unable to reset sensor.")

        if self._default_mode:
            self._mode(self._default_mode)
