#!/usr/bin/env python

# This program receives angle data from the simulated pbio motor driver. The
# driver outputs it at intervals of 40 ms (25 fps).

# Get live output from process.
while True:
    try:
        data = input()
    except EOFError:
        break
    # TODO: Serve the data values over socket here.
    # A react app could listen via socket io and update angles.
