"""Read and write to EV3 sensors through ev3dev sysfs."""
from os import listdir
from time import sleep, time


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


def number_of_sensors():
    """Return the number of sensors attached to the EV3."""
    return len(listdir('/sys/class/lego-sensor'))-2


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

    def __init__(self, port):
        """Initialize the sensor."""
        assert ord('1') <= port <= ord('4')
        self.port = port
        self.open_files()

    def open_files(self):
        """Open the sysfs files for this device."""
        self.path = get_sensor_path(self.port, self._ev3dev_driver_name)
        self.mode_file = open(self.path + 'mode', 'r+b')
        self.mode_now = read_str(self.mode_file)
        self.value_files = [open(self.path + 'value' + str(num), 'rb') for num in range(self._number_of_values)]

    def close_files(self):
        """Close the sysfs files for this device."""
        self.mode_file.close()
        for value_file in self.value_files:
            value_file.close()

    def reset(self):
        """Force sensor to reset as if disconnecting and reconnecting it."""
        # First, close all files for this sensor
        self.close_files()
        # Number of sensors before reset
        number_of_sensors_before = number_of_sensors()
        # Reset the sensor
        with open('/sys/class/lego-port/port' + chr(self.port-1) + '/mode', 'w') as rf:
            rf.write('auto')
        # Reset takes at least a second
        sleep(1)
        # Wait for the sensor to come back, up to a timeout. TODO: We should really be checking the actual port status, but that seems to prevent the sensor from coming back
        reset_time = time()
        while not number_of_sensors() == number_of_sensors_before:
            sleep(0.1)
            if time() - reset_time > 4:
                OSError("Unable to reset sensor")
        # Now that the sensor is almost back, wait another half second and then reinitialize the files
        sleep(0.5)
        self.open_files()

    def mode(self, mode_new):
        """Set the sensor mode. Not available to user."""
        if mode_new != self.mode_now:
            write_str(self.mode_file, mode_new)
            self.mode_now = mode_new

    def value(self, num):
        """Return value in sensor/valueX. Not available to user."""
        return read_int(self.value_files[num])
