from pybricks.tools import wait
from pybricks.parameters import ImageFile, Button, Port, Color
from pybricks.hubs import EV3Brick
from pybricks.ev3devices import InfraredSensor

from _ev3_motor_dc import motor_set_dc

# Load assets.
IMG_POS = ImageFile._ROTATE_CW18
IMG_NEG = ImageFile._ROTATE_CCW18
IMG_12 = ImageFile._APP_IR_CONTROL_12
IMG_34 = ImageFile._APP_IR_CONTROL_34

# Power level for all motors when on.
POWER = 100

# Load initial UI.
center_was_pressed = False
channel_12_active = True
ev3 = EV3Brick()


def draw_ui():
    ev3.screen.draw_image(0, 20, IMG_12 if channel_12_active else IMG_34)


# Wait for sensor to be attached.
draw_ui()
sensor = None
while sensor is None:
    try:
        sensor = InfraredSensor(Port.S4)
        sensor.buttons(1)
    except OSError:
        wait(10)


# Exit if sensor is unplugged or program ends.
while True:
    # Get button state for selected channel pair.
    channel_x = 1 if channel_12_active else 3
    channel_y = 2 if channel_12_active else 4
    pressed_x = sensor.buttons(channel_x)
    pressed_y = sensor.buttons(channel_y)

    # Use tank drive to set power levels for each motor.
    power = [
        POWER
        if Button.LEFT_UP in pressed_y
        else -POWER
        if Button.LEFT_DOWN in pressed_y
        else 0,
        POWER
        if Button.LEFT_UP in pressed_x
        else -POWER
        if Button.LEFT_DOWN in pressed_x
        else 0,
        POWER
        if Button.RIGHT_UP in pressed_x
        else -POWER
        if Button.RIGHT_DOWN in pressed_x
        else 0,
        POWER
        if Button.RIGHT_UP in pressed_y
        else -POWER
        if Button.RIGHT_DOWN in pressed_y
        else 0,
    ]

    # Blink if power set, otherwise steady green.
    if any(power):
        ev3.light.blink(Color.GREEN, [100, 100, 800, 100])
    else:
        ev3.light.on(Color.GREEN)

    # Apply power level to each motor, and draw visual indicator if moving.
    ev3.screen.draw_box(0, 0, 177, 16, fill=True, color=Color.WHITE)
    for m in range(len(power)):
        dc = motor_set_dc(m, power[m])
        if dc:
            ev3.screen.draw_image(20 + m * 40, 2, IMG_POS if dc > 0 else IMG_NEG)

    # Keep moving until IR button state changes.
    while pressed_x == sensor.buttons(channel_x) and pressed_y == sensor.buttons(
        channel_y
    ):
        wait(10)

        # While we wait, monitor center button to change channels.
        if Button.CENTER not in ev3.buttons.pressed():
            center_was_pressed = False
        elif not center_was_pressed:
            # Pressed now but not before, so toggle channel and restart.
            center_was_pressed = True
            channel_12_active = not channel_12_active
            draw_ui()
            break
