# Test all of the display methods listed in the official LEGO v1.0.0 docs.

import pybricks.ev3brick as ev3
from pybricks.parameters import Align

# Working directory is top-level tests directory
TEST_IMAGE = "../ports/pybricks/tests/ev3dev/v1/test.png"

ev3.display.clear()

ev3.display.text("test")
ev3.display.text("test", (0, 0))
ev3.display.text("test", location=(0, 0))

# NOTE: image() method implementation in v1.0.0 does not match official LEGO
# v1.0.0 docs. The LEGO docs list alignment= and coordinates= as separate
# parameters but the implementation shares a parameter location_or_alignment=
# that can be either type.
ev3.display.image(TEST_IMAGE)
ev3.display.image(TEST_IMAGE, Align.CENTER)
ev3.display.image(TEST_IMAGE, Align.CENTER, True)
ev3.display.image(TEST_IMAGE, Align.CENTER, clear=True)
ev3.display.image(TEST_IMAGE, (0, 0))
ev3.display.image(TEST_IMAGE, (0, 0), True)
ev3.display.image(TEST_IMAGE, (0, 0), clear=True)
ev3.display.image(TEST_IMAGE, clear=True)
