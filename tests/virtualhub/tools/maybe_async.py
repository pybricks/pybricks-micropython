from pybricks.pupdevices import Motor
from pybricks.parameters import Port
from pybricks.tools import wait, multitask, run_task, maybe_async

# Physically blocked in two directions.
ENDPOINT = 142
SPEED = 500
lever = Motor(Port.C)


@maybe_async
async def stall(speed):
    await lever.run_target(speed, 0)
    return await lever.run_until_stalled(speed)


@maybe_async
async def add(value, offset=0):
    await wait(10)
    return value + offset


# Equivalent to decorating, but without the syntax.
async def _multiply(value, factor=2):
    await wait(10)
    return value * factor


multiply = maybe_async(_multiply)


# Library functions can build on each other.
@maybe_async
async def add_then_multiply(value, offset, factor):
    return await multiply(await add(value, offset=offset), factor=factor)


@maybe_async
async def raise_error():
    await wait(10)
    raise ValueError("raised")


@maybe_async
async def report_active():
    return run_task()


class Mechanism:
    def __init__(self, motor, name):
        self.motor = motor
        self.name = name

    @maybe_async
    async def home(self, speed):
        await self.motor.run_target(speed, 0)
        angle = await self.motor.run_until_stalled(speed)
        return self.name, angle


mechanism = Mechanism(lever, "lever")

print(type(stall))

# Synchronous use runs to completion and returns the result.
print(stall(SPEED))
print(add(1))
print(add(1, offset=10))
print(add(value=2, offset=20))
print(multiply(3))
print(add_then_multiply(1, 2, factor=3))
print(mechanism.home(SPEED))
print(report_active())
print(run_task())

# Exceptions propagate and the run loop is cleaned up.
try:
    raise_error()
except ValueError as e:
    print(type(e), e.args)
print(run_task())

# Argument errors are raised directly.
try:
    add()
except TypeError as e:
    print(type(e))
try:
    maybe_async()
except TypeError as e:
    print(type(e))
try:
    maybe_async(add, add)
except TypeError as e:
    print(type(e))


# Asynchronous use returns an awaitable.
async def main():
    task = add(1)
    print(type(task))
    print(await task)

    print(await stall(SPEED))
    print(await add(1, offset=10))
    print(await multiply(3, factor=3))
    print(await add_then_multiply(1, 2, factor=3))
    print(await mechanism.home(SPEED))
    print(await report_active())
    print(await multitask(add(1, offset=1), multiply(2), add_then_multiply(1, 1, 1)))

    try:
        await raise_error()
    except ValueError as e:
        print(type(e), e.args)

    return "done"


print(run_task(main()))
print(run_task())
