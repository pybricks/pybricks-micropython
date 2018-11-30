# MIT License
#
# Copyright (c) 2017 David Lechner <david@lechnology.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

"""Internal module for interacting with sysfs"""

from os import listdir
from os import path


class Attribute():
    """A sysfs attribute

    Parameters:
        syspath (str): The path to the sysfs device node
        attr (str): The name of the sysfs attribute
        mode (str): The file mode to pass to ``open()``
    """
    def __init__(self, syspath, attr, mode):
        full_path = path.join(syspath,  attr)
        self.attr = open(full_path, mode)

    def read(self):
        """Reads the attribute value

        Returns:
            The value read from the attribute
        """
        self.attr.seek(0)
        return self.attr.read().strip()

    def write(self, value):
        """Writes the attribute value

        Parameters:
            value (str): The value to write
        """
        self.attr.write(value)


class IntAttribute(Attribute):
    """A sysfs attribute that has an integer value

    Parameters:
        syspath (str): The path to the sysfs device node
        attr (str): The name of the sysfs attribute
        mode (str): The file mode to pass to ``open()``
    """
    def __init__(self, syspath, attr, mode):
        super(IntAttribute, self).__init__(syspath, attr, mode)

    def read(self):
        """Reads the attribute value

        Returns:
            The value read from the attribute
        """
        return int(super(IntAttribute, self).read())

    def write(self, value):
        """Writes the attribute value

        Parameters:
            value (int): The value to write
        """
        super(IntAttribute, self).write(str(value))


class ListAttribute(Attribute):
    """A sysfs attribute that has a space-separated list of values

    Parameters:
        syspath (str): The path to the sysfs device node
        attr (str): The name of the sysfs attribute
        mode (str): The file mode to pass to ``open()``
    """
    def __init__(self, syspath, attr, mode):
        super(ListAttribute, self).__init__(syspath, attr, mode)

    def read(self):
        """Reads the attribute value

        Returns:
            A list of strings
        """
        return super(ListAttribute, self).read().split(' ')

    def write(self, value):
        """Raises ``RuntimeError``

        Writing to :py:class:`ListAttribute` is not allowed.
        """
        raise RuntimeError('Writing to ListAttribute is not supported')


def find_node(subsystem, address, driver):
    """Find a sysfs device node.

    Parameters:
        subsystem (str): The name of the subsystem.
        address (str): A value to match to the ``address`` attribute.
        driver (str): A value to match to the ``driver_name`` attribute.

    Returns:
        The path to the device or ``None`` if a match was not found.
    """
    syspath = path.join('/sys/class', subsystem)
    for node in listdir(syspath):
        node = path.join(syspath, node)
        addr = Attribute(node, 'address', 'r').read()
        if address != addr:
            continue
        drv = Attribute(node, 'driver_name', 'r').read()
        if driver != driver:
            continue
        return node
