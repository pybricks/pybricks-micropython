# Imports data.py, so that the other tests can check that a module is loaded
# once even when several modules import it.

import data

print(__name__, "loaded")


def describe():
    return "wrapper sees {}".format(data.value)
