# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: Any hub.

Description: Catch SystemExit from the stop button.
"""

from pybricks.tools import wait, StopWatch


for i in range(3):
    print("Press stop button or wait 5 seconds...")
    try:
        watch = StopWatch()
        while True:
            wait(10)
            if watch.time() > 5000:
                raise SystemExit
    except SystemExit:
        print("Caught SystemExit", i)

# Should print this after pressing the stop button 3 times.
print("Done!")
