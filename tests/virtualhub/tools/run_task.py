from pybricks.pupdevices import Motor
from pybricks.parameters import Port
from pybricks.tools import wait, multitask, run_task

# Physically blocked in two directions.
ENDPOINT = 142
SPEED = 500
lever = Motor(Port.C)


async def return_value(value):
    await wait(10)
    return value


async def return_nothing():
    await wait(10)


def generator_return(n):
    for i in range(n):
        yield i
    return n


async def raise_error():
    await wait(10)
    raise ValueError("raised")


async def check_active():
    return run_task()


async def nested_run_task():
    await wait(10)
    run_task(return_value(1))


async def stall():
    await lever.run_target(SPEED, 0)
    return await lever.run_until_stalled(SPEED)


# Without arguments, reports whether the run loop is active.
print(run_task())
print(run_task(check_active()))
print(run_task())

# Returns the result of the task.
print(run_task(return_value(42)))
print(run_task(return_value("text")))
print(run_task(return_nothing()))
print(run_task(generator_return(3)))
print(run_task(multitask(return_value(1), return_value(2))))

# Result of awaiting a built-in awaitable is passed through.
print(run_task(stall()))

# A task returning nothing after one that returned something gives None.
print(run_task(return_value(1)))
print(run_task(return_nothing()))

# Exceptions propagate and the run loop can be used again afterwards.
try:
    run_task(raise_error())
except ValueError as e:
    print(type(e), e.args)
print(run_task())
print(run_task(return_value(3)))

# Only one run loop at a time.
try:
    run_task(nested_run_task())
except RuntimeError as e:
    print(type(e), e.args)
print(run_task())


# User-defined wrapper that works in both sync and async code. This is what
# libraries had to do without pybricks.tools.maybe_async.
def sync_or_async(func):
    def wrapper(*args, **kwargs):
        coro = func(*args, **kwargs)
        return coro if run_task() else run_task(coro)

    return wrapper


@sync_or_async
async def wrapped_stall(speed):
    await lever.run_target(speed, 0)
    return await lever.run_until_stalled(speed)


@sync_or_async
async def wrapped_value(value, offset=0):
    await wait(10)
    return value + offset


# Synchronous use.
print(wrapped_stall(SPEED))
print(wrapped_value(1, offset=10))


# Asynchronous use.
async def main():
    print(await wrapped_stall(SPEED))
    print(await wrapped_value(2, offset=20))
    return "done"


print(run_task(main()))
