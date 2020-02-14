# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 Laurens Valk

# Import print for compatibility with 1.0 release
from builtins import print
from tools import wait, StopWatch
from utime import localtime, ticks_us


class DataLog():
    def __init__(self, *headers, name='log', timestamp=True, extension='csv'):

        # Make timestamp of the form yyyy_mm_dd_hh_mm_ss_uuuuuu
        if timestamp:
            y, mo, d, h, mi, s = localtime()[0:6]
            u = ticks_us() % 1000000
            stamp = '_{0}_{1:02d}_{2:02d}_{3:02d}_{4:02d}_{5:02d}_{6:06d}'.format(y, mo, d, h, mi, s, u)
        else:
            stamp = ''

        # Append extension and open
        self.file = open('{0}{1}.{2}'.format(name, stamp, ext), 'w+')

        # If column headers were given, print those as first line
        if len(headers) > 0:
            print(*headers, sep=', ', file=self.file)

    def log(self, *values):
        print(*values, sep=', ', file=self.file)

    def __repr__(self):
        self.file.seek(0, 0)
        return self.file.read()
