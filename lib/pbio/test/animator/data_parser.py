#!/usr/bin/env python

# This program receives angle data from the simulated pbio motor driver. The
# driver outputs it at intervals of 40 ms (25 fps).
#
# TODO: Visualize it for debugging and analysis to augment the logs.

with open("output.txt", "w") as f:
    while True:
        try:
            data = input()
        except EOFError:
            break
        f.write(data)
