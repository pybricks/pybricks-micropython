from pybricks.tools import Task, run_task


class CallCounter:
    def __init__(self):
        self.count = 0

    def __call__(self):
        self.count += 1


def count():
    """
    Count forever.
    """
    n = 0
    while True:
        yield n
        n += 1


def never_return(on_cancel):
    try:
        for i in count():
            yield i
    except GeneratorExit:
        on_cancel()


def return_after(n, on_cancel):
    try:
        for i in range(n):
            yield i
        return i
    except GeneratorExit:
        on_cancel()


def throw_after(n, msg, on_cancel):
    try:
        for i in range(n):
            yield i
        raise RuntimeError(msg)
    except GeneratorExit:
        on_cancel()


async def test_all_1():
    print("test_all_1")

    task1_cancel = CallCounter()
    task2_cancel = CallCounter()

    task1 = return_after(1, task1_cancel)
    task2 = return_after(2, task2_cancel)

    # should return [0, 1] - both allowed to complete
    print(await Task(task1, task2))

    # neither should be canceled
    print(task1_cancel.count, task2_cancel.count)

    # both should raise StopIteration because they are done

    try:
        next(task1)
    except StopIteration as e:
        print(type(e), e.args)

    try:
        next(task2)
    except StopIteration as e:
        print(type(e), e.args)


async def test_all_2():
    print("test_all_2")

    task1_cancel = CallCounter()
    task2_cancel = CallCounter()

    task1 = return_after(2, task1_cancel)
    task2 = throw_after(1, "task2", task2_cancel)

    # should throw RuntimeError("task2")
    try:
        await Task(task1, task2)
    except Exception as e:
        print(type(e), e.args)

    # task1 should be canceled
    print(task1_cancel.count, task2_cancel.count)

    # both should raise StopIteration because they are done

    try:
        next(task1)
    except StopIteration as e:
        print(type(e), e.args)

    try:
        next(task2)
    except StopIteration as e:
        print(type(e), e.args)


async def test_race_1():
    print("test_race_1")

    task1_cancel = CallCounter()
    task2_cancel = CallCounter()

    task1 = return_after(1, task1_cancel)
    task2 = return_after(2, task2_cancel)

    # returns [0, None] - only first completes
    print(await Task(task1, task2, race=True))

    # task2 should be canceled
    print(task1_cancel.count, task2_cancel.count)

    # both should raise StopIteration because they are done/canceled

    try:
        next(task1)
    except StopIteration as e:
        print(type(e), e.args)

    try:
        next(task2)
    except StopIteration as e:
        print(type(e), e.args)


async def test_race_2():
    print("test_race_2")

    task1_cancel = CallCounter()
    task2_cancel = CallCounter()

    task1 = return_after(2, task1_cancel)
    task2 = throw_after(1, "task2", task2_cancel)

    # should throw RuntimeError("task2")
    try:
        await Task(task1, task2, race=True)
    except Exception as e:
        print(type(e), e.args)

    # task1 should be canceled
    print(task1_cancel.count, task2_cancel.count)

    # both should raise StopIteration because they are done

    try:
        next(task1)
    except StopIteration as e:
        print(type(e), e.args)

    try:
        next(task2)
    except StopIteration as e:
        print(type(e), e.args)


async def main():
    await test_all_1()
    await test_all_2()
    await test_race_1()
    await test_race_2()


# run as fast as possible for CI
run_task(main(), loop_time=0)
