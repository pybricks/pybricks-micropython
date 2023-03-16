def all(*tasks):
    """
    Runs each task in parallel and waits for all to complete.

    Args:
        tasks: one or more tasks/generator functions to run in parallel.

    Returns:
        A list containing the return value of each task in the same order
        as *tasks*.

    Example::

        # generator syntax
        def example1():
            result1, result2 = yield from tasks.all(task1(), task2())
            ...

        # async/await syntax
        async def example2():
            result1, result2 = await tasks.all(task1(), task2())
            ...

        # return value can be ignored
        async def example3():
            await tasks.all(task1(), task2())
            ...
    """
    r = [None] * len(tasks)
    pending = {i: t for i, t in enumerate(tasks)}
    done = []

    try:
        while pending:
            for i, t in pending.items():
                try:
                    next(t)
                except StopIteration as ex:
                    r[i] = ex.value
                    done.append(i)

            # can't modify pending while we are iterating it so defer to here
            for i in done:
                del pending[i]

            done.clear()

            yield
    finally:
        # In case of crash in one task, all other unfinished tasks are
        # canceled. Calling close() on finished tasks is a nop.
        for t in tasks:
            t.close()

    return r


def race(*tasks):
    """
    Runs each task in parallel and waits for the first one to complete. After
    the first task completes, all other tasks will be cleaned up by calling the
    generator ``close()`` function.

    Important: if you want to know which task won the race, all tasks must
    return a value that is not ``None``.

    Args:
        tasks: One or more tasks/generator functions to run in parallel.

    Returns:
        A list of values corresponding to each task in the same order as
        *tasks*. All values will be ``None`` except for the winner of the
        race.
    """
    r = [None] * len(tasks)

    try:
        while True:
            for i, t in enumerate(tasks):
                next(t)
            yield
    except StopIteration as ex:
        # winner of the race - save the return value
        r[i] = ex.value
    finally:
        for t in tasks:
            t.close()

    return r


def run(task, loop_time=10):
    """
    Runs a task in the main event loop.

    This will run one iteration of the task every *loop_time*.

    Args:
        loop_time: The target loop time in milliseconds.

    Basic usage::

        async def main():
            await ...
            ...

        tasks.run(main())

    To run more than one task, use :meth:`tasks.all`::

        tasks.run(tasks.all(task_1(), task_2()))
    """
    from gc import collect
    from pybricks.tools import _set_run_loop_active, StopWatch, wait

    _set_run_loop_active(True)

    try:
        timer = StopWatch()
        start = timer.time()

        # TODO: keep track of loop time stats and provide API to user

        for _ in task:
            # needed here so that wait() is blocking
            _set_run_loop_active(False)

            # garbage collect to keep GC high watermark to a minimum
            collect()

            # GC can take many milliseconds when lots of RAM is used so we need
            # to only wait the time remaining after user code and GC.
            val = max(0, loop_time - (timer.time() - start))
            wait(val)

            start += loop_time

            _set_run_loop_active(True)
    finally:
        _set_run_loop_active(False)
