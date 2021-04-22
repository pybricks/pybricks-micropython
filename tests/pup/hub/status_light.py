# Should be able to display each color (note: colors without 100% saturation
# will not appear correctly due to physics).

# Press the stop button to advance to the next color

from pybricks.hubs import *  # noqa: F401, F403
from pybricks.parameters import Color
from pybricks.tools import wait

# sneaky way to get XyzHub class without knowning which hub
hub = next(v for k, v in globals().items() if k.endswith("Hub"))()

for c in Color:
    print("Color:", c)
    hub.light.on(Color[c])
    try:
        while True:
            wait(10)
    except SystemExit:
        continue

hub.light.off()
print("...done")
