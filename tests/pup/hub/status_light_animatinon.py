# Should be able to smoothly transition between all colors.

from pybricks.hubs import *
from pybricks.parameters import Color
from pybricks.tools import wait

# sneaky way to get XyzHub class without knowning which hub
hub = next(v for k, v in globals().items() if k.endswith("Hub"))()

# Every possible hue with 100% saturation, 100% brightness.
# Note: In real programs, use every 2 or 3 degrees to save memory.
rainbow = [Color.BLUE >> d for d in range(360)]

hub.light.animate(rainbow, 100)

while True:
    wait(10)
