# SPDX-License-Identifier: MIT
# Copyright (c) 2019-2020 The Pybricks Authors

"""Classes for LEGO MINDSTORMS NXT Devices."""


from nxtdevices_c import *
from pybricks.iodevices import AnalogSensor


class VernierAdapter(AnalogSensor):

    def __init__(self, port, conversion=None):
        # create AnalogSensor object
        super().__init__(port)

        # Store conversion function if given
        if conversion is not None:
            self.conversion = conversion

        # Verify that conversion is valid
        try:
            for v in (0, 10, 1000, 3000, 5000):
                _ = self.conversion(v)
        except Exception as e:
            print(
                """\nThere is an error in the conversion function. """
                """Make sure it has the following form:\n"""
                """\ndef my_conversion_function(voltage):"""
                """\n    value = voltage * 3  """
                """# This is just an example. Use your own formula here."""
                """\n    return value\n"""
                """\nThe technical description of the error is as follows:\n"""
            )
            raise e

    def conversion(self, voltage):
        return voltage

    def value(self):
        return self.conversion(self.voltage())
