# SPDX-License-Identifier: MIT
# Copyright (c) 2020 The Pybricks Authors

"""
Hardware Module: Any hub.

Description: Display each color. Colors without 100% saturation
will not appear correctly due to physics.
"""

from pybricks.hubs import ThisHub
from pybricks.parameters import Color
from pybricks.tools import wait

# Initialize this hub, whichever it is.
hub = ThisHub()

for c in Color:
    print("Color:", c)
    hub.light.on(Color[c])
    wait(1000)

hub.light.off()
print("...done")
