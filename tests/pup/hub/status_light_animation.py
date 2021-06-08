# Should be able to smoothly transition between all colors.

from pybricks.hubs import ThisHub
from pybricks.parameters import Color
from pybricks.tools import wait

# Initialize this hub, whichever it is.
hub = ThisHub()

# Every possible hue with 100% saturation, 100% brightness.
# Note: In real programs, use every 2 or 3 degrees to save memory.
rainbow = [Color.BLUE >> d for d in range(360)]

# Start the animation.
INTERVAL = 50
hub.light.animate(rainbow, INTERVAL)

# Wait for two whole cycles.
wait(2 * INTERVAL * len(rainbow))
