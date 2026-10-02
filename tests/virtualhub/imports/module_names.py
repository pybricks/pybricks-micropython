# Checks that a program downloaded as several modules keeps them apart, and
# that each one knows its own name.

import data
import wrapper
from data import value

# The main script is always __main__, no matter what its file is called.
print("main is", __name__)

# Imported modules keep the name they were downloaded under.
print("imported", data.__name__, "and", wrapper.__name__)

# Data crosses module boundaries, both directly and through a function.
print("value is", data.value, "or", value)
print(wrapper.describe())

# data is imported here and by wrapper, but it runs once and both get the
# same module object.
import data as same_data

print("same module:", same_data is data)

# The hub has no file system, so modules have no file name to report.
try:
    print(__file__)
except NameError:
    print("no __file__")

try:
    print(data.__file__)
except AttributeError:
    print("no data.__file__")
