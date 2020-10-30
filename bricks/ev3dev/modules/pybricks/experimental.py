"""The experimental module contains unstable APIs for development and testing.
"""

from _pybricks.experimental import pthread_raise, Matrix, Vector, UnitVector
from _thread import start_new_thread, get_ident, allocate_lock
from usignal import pthread_kill, SIGUSR2

# FIXME: Make generic fix for running init of module from builtin package
from _pybricks.experimental import __init__ as experimental_init

experimental_init()


def run_parallel(*args):
    """Runs functions in parallel and waits for all to complete.

    If any function has an unhandled exception, all other functions will be
    stopped (by raising the ``SystemExit`` exception).

    Arguments:
        * (callable):
            Two or more functions with no required parameters.

    Returns:
        A dictionary mapping the functions to their return value (or a
        ``SystemExit`` exception object if the function was interrupted).

    Raises:
        TypeError:
            Fewer than 2 arguments or one of the arguments is not callable.
        RuntimeError:
            Any of the functions had an unhandled exception. The argument at
            index 1 will be a dictionary mapping the functions to the
            exceptions (or result if a function finished before the unhandled
            exception in another function).

    Example::

        from pybricks.experimental import run_parallel
        from pybricks.tools import wait

        def task1():
            wait(1000)
            return 'OK1'

        def task2():
            wait(500)
            return 'OK2'

        result = run_parallel(task1, task2)
        print('task1:', repr(result[task1]))
        print('task2:', repr(result[task2]))

        # prints:
        # task1: 'OK1'
        # task2: 'OK2'
    """
    if len(args) < 2:
        raise TypeError("requires at least 2 arguments")
    for n, arg in enumerate(args):
        if not callable(arg):
            raise TypeError("argument {} is not callable".format(n))

    ids = set()
    lock = allocate_lock()
    lock.acquire()
    results = {}
    unhandled_ex = False

    def wrapper(func):
        nonlocal unhandled_ex
        try:
            results[func] = func()
        except BaseException as ex:
            results[func] = ex
            if isinstance(ex, Exception):
                unhandled_ex = True
        finally:
            ids.remove(get_ident())
            if not ids:
                lock.release()
            elif unhandled_ex:
                id = next(iter(ids))
                pthread_raise(id, SystemExit())
                pthread_kill(id, SIGUSR2)

    for arg in args:
        ids.add(start_new_thread(wrapper, (arg,)))

    lock.acquire()
    if any(isinstance(x, Exception) for x in results.values()):
        raise RuntimeError("An unhandled exception occurred in run_parallel", results)
    return results
