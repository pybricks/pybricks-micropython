"""Read and write to EV3 sensors through ev3dev sysfs."""
from os import listdir


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


class Mode():
    """Enumeration of sensor modes. Not available to user."""
    color = 'COL-COLOR'
    ambient = 'COL-AMBIENT'
    reflectedraw = 'REF-RAW'
    rgb = 'RGB-RAW'
    proximity = 'IR-PROX'
    beacon = 'IR-SEEK'
    remote = 'IR-REMOTE'


class Ev3devSensor():
    """Base class for ev3dev sensors operating through sysfs."""
    _ev3dev_driver_name = 'none'
    _number_of_values = 1

    def __init__(self, port):
        """Initialize the sensor."""
        self.path = get_sensor_path(port, self._ev3dev_driver_name)
        self.file_mode = open(self.path + 'mode', 'r+b')
        self.mode_now = read_str(self.file_mode)

        # Open value files as relevant
        self.file_value0 = open(self.path + 'value0', 'rb')
        if self._number_of_values == 1:
            return
        self.file_value1 = open(self.path + 'value1', 'rb')
        self.file_value2 = open(self.path + 'value2', 'rb')
        if self._number_of_values == 3:
            return
        self.file_value3 = open(self.path + 'value3', 'rb')
        self.file_value4 = open(self.path + 'value4', 'rb')
        self.file_value5 = open(self.path + 'value5', 'rb')
        self.file_value6 = open(self.path + 'value6', 'rb')
        self.file_value7 = open(self.path + 'value7', 'rb')

    def mode(self, new_mode):
        """Set the sensor mode. Not available to user."""
        if new_mode != self.mode_now:
            write_str(self.file_mode, new_mode)
            self.mode_now = new_mode

    def value0(self):
        """Return value in sensor/value0. Not available to user."""
        return read_int(self.file_value0)

    def value1(self):
        """Return value in sensor/value1. Not available to user."""
        return read_int(self.file_value1)

    def value2(self):
        """Return value in sensor/value2. Not available to user."""
        return read_int(self.file_value2)

    def value3(self):
        """Return value in sensor/value3. Not available to user."""
        return read_int(self.file_value3)

    def value4(self):
        """Return value in sensor/value4. Not available to user."""
        return read_int(self.file_value4)

    def value5(self):
        """Return value in sensor/value5. Not available to user."""
        return read_int(self.file_value5)

    def value6(self):
        """Return value in sensor/value6. Not available to user."""
        return read_int(self.file_value6)

    def value7(self):
        """Return value in sensor/value7. Not available to user."""
        return read_int(self.file_value7)
