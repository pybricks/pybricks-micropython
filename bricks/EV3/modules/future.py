"""Experimental features to be rewritten in C code at a later stage."""

from utime import sleep_ms # Use mp_hal instead for in the C implementation

def wait(*args):
    """Wait for a specified amount of time or a condition."""
    nargs = len(args)
    first = args[0]
    # If it just a number, wait for that many miliseconds
    if nargs == 1 and (type(first) == int or type(first) == float):
        sleep_ms(int(args[0]))
    # Otherwise, wait until the specified condition holds
    else:
        # First argument is the generator
        func = first
        if nargs == 1:
            # If no operator/compare value is specified, check for truth
            operator = '=='
            compare = True
        else:
            # The second argument is operator, third is compare value
            operator = args[1]
            compare = args[2]

        # Wait until condition is triggered
        if operator == '==':
            while(func() != compare):
                sleep_ms(10)
        elif operator == '!=':
            while(func() == compare):
                sleep_ms(10)
        elif operator == '>':
            while(func() <= compare):
                sleep_ms(10)
        elif operator == '<':
            while(func() >= compare):
                sleep_ms(10)
        elif operator == '<=':
            while(func() > compare):
                sleep_ms(10)
        elif operator == '>=':
            while(func() < compare):
                sleep_ms(10)
        elif operator == 'in':
            while(func() not in compare):
                sleep_ms(10)
        elif operator == 'not in':
            while(func() in compare):
                sleep_ms(10)
