from pybricks.pupdevices import Motor
from pybricks.parameters import Port
from pybricks.tools import wait, StopWatch

from errno import ETIMEDOUT
from os import getenv

# environment variables allow running one-off tests
TARGET_ANGLE = int(getenv("TARGET_ANGLE", 1000))
SPEED = int(getenv("SPEED", 500))
TIMEOUT = int(getenv("TIMEOUT", 10000))
TOLERANCE = int(getenv("TOLERANCE", 5))

motor = Motor(Port.A)
watch = StopWatch()

# In the future, replace with async, but this will do fine today
motor.run_target(speed=SPEED, target_angle=TARGET_ANGLE, wait=False)

while not motor.done():

    if watch.time() > TIMEOUT:
        raise OSError(ETIMEDOUT)

    wait(10)

# Various checks
assert TARGET_ANGLE - TOLERANCE <= motor.angle() <= TARGET_ANGLE + TOLERANCE

# No exceptions or output means success
