# SPDX-License-Identifier: MIT
# Copyright (c) 2018 Laurens Valk

# TODO: This is a quick fix for the purpose of proposing the API change. If moving forward with this, we can properly adjust the implementation at c level instead.
from tools import wait, StopWatch

from sys import stderr
from builtins import print as builtinprint

def print(*args, **kwargs):
    """Print a message on the IDE terminal."""
    builtinprint(*args, file=stderr, **kwargs)
