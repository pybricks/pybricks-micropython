# Checks imports that are not part of the downloaded program. Names that the
# firmware provides are resolved there, and anything left over is an error.

from pybricks.tools import wait

wait(0)
print("firmware import ok")

try:
    import nope
except ImportError as e:
    print("ImportError:", e)

# Module names are flat on the hub, so there is nothing to be relative to.
try:
    from . import data
except NotImplementedError as e:
    print("NotImplementedError:", e)
