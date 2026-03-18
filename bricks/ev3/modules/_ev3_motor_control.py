from pybricks.tools import wait
from pybricks.parameters import ImageFile, Button, Port, Color
from pybricks.hubs import EV3Brick
from pybricks.ev3devices import Motor

ev3 = EV3Brick()

# True means A&D pair is active. False means B&C pair is active.
pair_ad_active = False

# Re-index motors by port for re-initialization on the fly.
motors = {
    Port.A: None,
    Port.B: None,
    Port.C: None,
    Port.D: None,
}

POWER = 100

# Preload to avoid repeated allocation.
IMG_POS = ImageFile._ROTATE_CW18
IMG_NEG = ImageFile._ROTATE_CCW18
IMG_AD = ImageFile._APP_MOTOR_CONTROL_AD
IMG_BC = ImageFile._APP_MOTOR_CONTROL_BC


# Wrapper to set DC and reinitialize motor on the fly.
def set_dc(port, dc):
    # If there was a motor, close if it got unplugged.
    if motors[port] is not None:
        try:
            motors[port].angle()
        except OSError:
            motors[port].close()
            motors[port] = None
            pass

    # If there isn't a motor, try to initialize it now.
    if motors[port] is None:
        try:
            motors[port] = Motor(port)
        except OSError:
            pass

    # Drive the motor if there is one.
    if motors[port]:
        try:
            motors[port].dc(dc)
            # On success, indicate the active direction on the display.
            image = IMG_POS if dc > 0 else IMG_NEG if dc < 0 else None
            if image:
                index = ord(repr(port)[5]) - ord("A")
                ev3.screen.draw_image(20 + index * 40, 2, image)
        except OSError:
            motors[port].close()
            motors[port] = None


def draw_ui():
    ev3.screen.clear()
    file = IMG_AD if pair_ad_active else IMG_BC
    ev3.screen.draw_image(0, 20, file)
    ev3.screen.draw_box(0, 0, 177, 16, fill=True, color=Color.WHITE)


while True:
    draw_ui()
    ev3.light.on(Color.GREEN)

    # Wait for anything to happen
    while not (pressed := ev3.buttons.pressed()):
        wait(10)

    # Center button toggles motor pair
    if Button.CENTER in pressed:
        pair_ad_active = not pair_ad_active
        draw_ui()
        while any(ev3.buttons.pressed()):
            wait(10)
        continue

    # Determine motor power for vertical buttons.
    vertical_dc = 0
    if Button.UP in pressed:
        vertical_dc += POWER
    if Button.DOWN in pressed:
        vertical_dc -= POWER

    # Determine motor power for horizontal buttons.
    horizontal_dc = 0
    if Button.RIGHT in pressed:
        horizontal_dc += POWER
    if Button.LEFT in pressed:
        horizontal_dc -= POWER

    # Activity heartbeat
    ev3.light.blink(Color.GREEN, [100, 100, 800, 100])

    # Drive one or two motors.
    set_dc(Port.A if pair_ad_active else Port.B, vertical_dc)
    set_dc(Port.D if pair_ad_active else Port.C, horizontal_dc)

    # Keep going until button state changes.
    while ev3.buttons.pressed() == pressed:
        wait(10)

    # Stop all motors.
    for motor in motors.values():
        try:
            motor.stop()
        except (OSError, AttributeError):
            pass
