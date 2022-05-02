from pybricks.hubs import NXTBrick
from pybricks.tools import wait

hub = NXTBrick()

last = True

while True:

    now = bool(hub.buttons.pressed())

    if last != now:
        last = now
        if now:
            print("You pressed a button!")
        else:
            print("Released!")
    wait(10)
