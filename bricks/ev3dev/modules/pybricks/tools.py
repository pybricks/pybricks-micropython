# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 Laurens Valk

from tools import wait, StopWatch

# Import print for compatibility with 1.0 release
from builtins import print


class DataLog():
    def __init__(self, path, header=None):
        self.path = path
        self.file = open(self.path, 'w')
        if header:
            self.file.write('{0}\n'.format(header))

    def log(self, *args):
        line = ', '.join(str(a) for a in args)
        self.file.write('{0}\r\n'.format(line))

    def show(self):
        self.file.close()
        print('Contents of {0}:'.format(self.path))
        with open(self.path, 'r') as of:
            print(of.read())
        self.file = open(self.path, 'a')
