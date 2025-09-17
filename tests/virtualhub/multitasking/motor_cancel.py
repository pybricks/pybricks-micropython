from pybricks.pupdevices import Motor
from pybricks.parameters import Port, Direction
from pybricks.robotics import DriveBase
from pybricks.tools import wait, multitask, run_task

ENDPOINT = 142
SPEED = 500


def is_close(motor, target):
    return abs(motor.angle() - target) < 5


# Spins freely.
motor = Motor(Port.A)

# Physically blocked in two directions.
lever = Motor(Port.C)


def reset():
    for m in (motor, lever):
        m.run_target(SPEED, 0)
        assert is_close(m, 0)


reset()

# Should block until endpoint.
assert lever.run_until_stalled(SPEED) == ENDPOINT
assert lever.angle() == ENDPOINT

# Should block until close to target
lever.run_target(SPEED, 0)
assert is_close(lever, 0)


async def stall():
    # Should return None for most movements.
    ret = await lever.run_target(SPEED, -90)
    assert is_close(lever, -90)
    assert ret is None

    # Should return value at end of stall awaitable.
    stall_angle = await lever.run_until_stalled(SPEED)
    assert stall_angle == ENDPOINT


run_task(stall())


def is_coasting(motor):
    return motor.load() == 0


# Confirm that stop() coasts the motor.
motor.run_angle(SPEED, 360)
assert not is_coasting(motor)
motor.stop()
assert is_coasting(motor)
reset()


async def par1(expect_interruption=False):
    for i in range(4):
        await motor.run_angle(SPEED, 90)
    if expect_interruption:
        raise RuntimeError("Expected interruption, so shold never see this.")


async def par2():
    await wait(100)


# Let the motor run on its own.
reset()
run_task(par1())
assert not is_coasting(motor)
assert is_close(motor, 360)

# Let the motor run in parallel to a task that does not affect it.
reset()
run_task(multitask(par1(), par2()))
assert not is_coasting(motor)
assert is_close(motor, 360)

# Let the motor run in parallel to a short task as a race. This should cancel
# the motor task early and coast it.
reset()
run_task(multitask(par1(True), par2(), race=True))
assert is_coasting(motor)
assert not is_close(motor, 360)


reset()


async def par3():
    await motor.run_target(SPEED, 36000)
    # We should never make it, but stop waiting and proceed instead.
    assert not is_close(motor, 36000)
    print("motor movement awaiting was cancelled")


async def par4():
    await wait(500)
    print("Going to take over the motor.")
    await motor.run_target(SPEED, 90)
    print("Finished turning after take over.")


run_task(multitask(par3(), par4()))
