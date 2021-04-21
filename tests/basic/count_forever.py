# Should be able to print forever in a tight loop.

# Used to reproduce bug on https://github.com/pybricks/support/issues/163
# and https://github.com/pybricks/support/issues/36

# If it gets to at least 100000, things are looking good.


def count():
    n = 0
    while True:
        yield n
        n += 1


for n in count():
    print("count:", n)
