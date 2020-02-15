import uos

from pybricks.experimental import run_parallel
from pybricks.tools import wait

if uos.getenv('PYBRICKS_BUILD_ENV') == 'docker-armel':
    # qemu-user-static has issues with threads
    print('SKIP')
    raise SystemExit


def task1():
    wait(1000)
    return 'OK1'


def task2():
    wait(500)
    return 'OK2'


def task3():
    # Unhandled Exception should interrupt all other tasks
    raise Exception('oops')


def task4():
    # Unhandled BaseException does not interrupt other tasks
    raise SystemExit


result = run_parallel(task1, task2)
print(repr(result[task1]))
print(repr(result[task2]))

try:
    run_parallel(task1, task2, task3)
except RuntimeError as ex:
    print(ex.args[0])  # message
    print(repr(ex.args[1][task1]))
    # sometimes task2 finishes before being interrupted
    # print(repr(ex.args[1][task2]))
    print(repr(ex.args[1][task3]))

result = run_parallel(task1, task2, task4)
print(repr(result[task1]))
print(repr(result[task2]))
print(repr(result[task4]))
