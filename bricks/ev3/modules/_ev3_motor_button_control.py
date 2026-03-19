from pybricks.tools import wait
from pybricks.parameters import ImageFile, Button, Port, Color
from pybricks.hubs import EV3Brick
from pybricks.ev3devices import Motor

from _ev3_motor_dc import motor_set_dc


# True means A&D pair is active. False means B&C pair is active.
pair_ad_active = False

# Power used for all motors if active.
POWER = 100

# Preload to avoid repeated allocation.
IMG_POS = ImageFile._ROTATE_CW18
IMG_NEG = ImageFile._ROTATE_CCW18
IMG_AD = ImageFile._APP_MOTOR_CONTROL_AD
IMG_BC = ImageFile._APP_MOTOR_CONTROL_BC

# UI is one of two prebuilt images from lms2012.
ev3 = EV3Brick()


def draw_ui():
    file = IMG_AD if pair_ad_active else IMG_BC
    ev3.screen.draw_image(0, 20, file)
    ev3.screen.draw_box(0, 0, 177, 16, fill=True, color=Color.WHITE)


draw_ui()

while True:
    # Start with light on and motors off.
    draw_ui()
    ev3.light.on(Color.GREEN)
    for m in range(4):
        motor_set_dc(m, 0)

    # Wait for anything to happen.
    while not (pressed := ev3.buttons.pressed()):
        wait(10)

    # Center button toggles motor pair
    if Button.CENTER in pressed:
        pair_ad_active = not pair_ad_active
        draw_ui()
        while any(ev3.buttons.pressed()):
            wait(10)
        continue

    # Determine motor power for horizontal buttons.
    dc_x = 0
    if Button.RIGHT in pressed:
        dc_x += POWER
    if Button.LEFT in pressed:
        dc_x -= POWER

    # Determine motor power for vertical buttons.
    dc_y = 0
    if Button.UP in pressed:
        dc_y += POWER
    if Button.DOWN in pressed:
        dc_y -= POWER

    # Activity heartbeat.
    if dc_x or dc_y:
        ev3.light.blink(Color.GREEN, [100, 100, 800, 100])
    else:
        ev3.light.on(Color.GREEN)

    # Drive one or two motors.
    output = [0, 0, 0, 0]
    index_x = 3 if pair_ad_active else 2
    index_y = 0 if pair_ad_active else 1
    output[index_x] = motor_set_dc(index_x, dc_x)
    output[index_y] = motor_set_dc(index_y, dc_y)

    # Draw output indicators if active.
    for m in range(len(output)):
        if output[m]:
            ev3.screen.draw_image(20 + m * 40, 2, IMG_POS if output[m] > 0 else IMG_NEG)

    # Keep going until button state changes.
    while ev3.buttons.pressed() == pressed:
        wait(10)
