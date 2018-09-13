from _motor import Internal
from test import External
from _constants import *

int_test = Internal(PORT_A)
ext_test = External(PORT_C)

int_test.duty(50) # works
ext_test.duty(50) # AttributeError: 'External' object has no attribute 'duty'

