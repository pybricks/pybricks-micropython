from uerrno import EAGAIN

from pybricks.experimental import create_echo_stream
from pybricks.tools import wait

e = create_echo_stream(32, 32)

try:
    e.read(1)
    assert False
except OSError as ex:
    assert ex.errno == EAGAIN


try:
    e.write(b"\x00" * 33)
    assert False
except ValueError as ex:
    assert "too big" in ex.args[0]


e.write(b"\x00" * 32)

# TODO: how to ensure contiki event loop did not run?
try:
    e.write(b"\x00")
    assert False
except OSError as ex:
    assert ex.errno == EAGAIN

# TODO: how to ensure contiki event loop ran?
assert e.write(b"\x00" * 32) == 32

try:
    e.write(b"\x00")
    assert False
except OSError as ex:
    assert ex.errno == EAGAIN

try:
    e.read(33)
    assert False
except ValueError as ex:
    assert "n must be <" in ex.args[0]

assert e.read(32) == b"\x00" * 32
assert e.read(32) == b"\x00" * 32

print("OK")
