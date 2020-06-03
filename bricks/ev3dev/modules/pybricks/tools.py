# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 The Pybricks Authors

# Expose method and class written in C
from tools import wait, StopWatch

# Imports for DataLog implementation
from utime import localtime, ticks_us


class DataLog:
    def __init__(self, *headers, name="log", timestamp=True, extension="csv", append=False):

        # Make timestamp of the form yyyy_mm_dd_hh_mm_ss_uuuuuu
        if timestamp:
            y, mo, d, h, mi, s = localtime()[0:6]
            u = ticks_us() % 1000000
            stamp = "_{0}_{1:02d}_{2:02d}_{3:02d}_{4:02d}_{5:02d}_{6:06d}".format(
                y, mo, d, h, mi, s, u
            )
        else:
            stamp = ""

        # File write mode
        mode = "a+" if append else "w+"

        # Append extension and open
        self.file = open("{0}{1}.{2}".format(name, stamp, extension), mode)

        # Get length of existing contents
        self.file.seek(0, 2)
        length = self.file.tell()

        # If column headers were given and we are at the start of the file, print headers as first line
        if len(headers) > 0 and length == 0:
            print(*headers, sep=", ", file=self.file)

    def log(self, *values):
        print(*values, sep=", ", file=self.file)

    def __repr__(self):
        self.file.seek(0, 0)
        return self.file.read()
