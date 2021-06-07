# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: Any hub.

Description: Print a long time in a tight loop.
"""

def count():
    n = 0
    while True:
        yield n
        n += 1


for n in count():
    print("count:", n)
    if n >= 10000:
        break
