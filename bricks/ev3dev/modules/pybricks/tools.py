# SPDX-License-Identifier: MIT
# Copyright (c) 2018-2020 Laurens Valk

# Import print for compatibility with 1.0 release
from builtins import print
from tools import wait, StopWatch


class DataLog():
    def __init__(self, path, *headers):
        self.file = open(path, 'w+')
        if len(headers) > 0:
            print(*headers, sep=', ', file=self.file)

    def log(self, *args):
        print(*args, sep=', ', file=self.file)

    def __repr__(self):
        self.file.seek(0, 0)
        return self.file.read()
