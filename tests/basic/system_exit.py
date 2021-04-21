# Should be able to catch SystemExit from the stop button.

from pybricks.tools import wait

for i in range(3):
    print("press stop button...")
    try:
        while True:
            wait(10)
    except SystemExit:
        print("caught SystemExit", i)

# Should print this after pressing the stop button 3 times.
print("...done")
