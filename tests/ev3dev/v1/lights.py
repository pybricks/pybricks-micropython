import pybricks.ev3brick as ev3
from pybricks.parameters import Color
from pybricks.tools import wait

SYSFS = '/sys/class/leds/led{}:{}:brick-status/{}'
LEDS = [
    (0, 'green'),
    (0, 'red'),
    (1, 'green'),
    (1, 'red'),
]


# since pbio library is just seeking, we need to clear the file to verify
# that a new value was written
def clear_sysfs(id, color, attr):
    with open(SYSFS.format(id, color, attr), 'w') as f:
        pass


def print_sysfs(id, color, attr):
    with open(SYSFS.format(id, color, attr), 'r') as f:
        print(f.read())


def test(color_arg):
    for id, color in LEDS:
        clear_sysfs(id, color, 'brightness')

    ev3.light(color_arg)

    wait(100)  # change doesn't take effect until background thread runs

    for id, color in LEDS:
        print_sysfs(id, color, 'brightness')


# trigger should be intialized to "none"
for id, color in LEDS:
    print_sysfs(id, color, 'trigger')


test(Color.RED)
test(Color.GREEN)
test(None)
