from pybricks.pupdevices import Motor
from pybricks.parameters import Port

m = Motor(Port.A)

# All objects should be hashable.
# Regression test for https://github.com/pybricks/support/issues/876.
print(type(hash(m)) is int)
