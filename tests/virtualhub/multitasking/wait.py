from pybricks.tools import multitask, run_task, wait, StopWatch

watch = StopWatch()
DELAY = 100

# Should block
watch.reset()
wait(DELAY)
assert watch.time() == DELAY


async def one_wait():
    # Forgot await, so should not wait.
    watch.reset()
    wait(DELAY)
    assert watch.time() == 0

    # Should await.
    watch.reset()
    await wait(DELAY)
    assert watch.time() == DELAY

    # Await object
    watch.reset()
    it = wait(DELAY)
    await it
    assert watch.time() == DELAY


run_task(one_wait())


async def main1():
    print("started main1")
    await wait(DELAY)
    print("completed main1")


async def main2():
    print("started main2")
    await wait(DELAY * 2)
    print("completed main2")


# Should get all outputs
watch.reset()
run_task(multitask(main1(), main2()))
assert watch.time() == DELAY * 2

# Only one task completes.
watch.reset()
run_task(multitask(main1(), main2(), race=True))
assert watch.time() == DELAY
