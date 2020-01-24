# SPDX-License-Identifier: MIT
# Copyright (c) 2018 Laurens Valk

# TODO: This is a quick fix for the purpose of proposing the API change. If moving forward with this, we can properly adjust the implementation at c level instead.
from tools import wait, StopWatch

from sys import stderr
from builtins import print as builtinprint


def print(*args, **kwargs):
    """Print a message on the IDE terminal."""
    builtinprint(*args, file=stderr, **kwargs)


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
