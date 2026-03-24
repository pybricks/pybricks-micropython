from pybricks.hubs import EV3Brick
from pybricks.ev3devices import (
    Motor,
    ColorSensor as EV3ColorSensor,
    TouchSensor as EV3TouchSensor,
    GyroSensor,
    UltrasonicSensor,
    InfraredSensor,
)
from pybricks.nxtdevices import ColorSensor as NXTColorSensor, LightSensor, SoundSensor
from pybricks.parameters import Button, Port, ImageFile
from pybricks.tools import wait


# Load and decompress images once.
IMG_EV3_COLOR_AMBIENT = ImageFile._PORT_VIEW_EV3_COLOR_AMBIENT
IMG_EV3_COLOR_COLOR = ImageFile._PORT_VIEW_EV3_COLOR_COLOR
IMG_EV3_COLOR_REFLECTION = ImageFile._PORT_VIEW_EV3_COLOR_REFLECTION
IMG_EV3_GYRO = ImageFile._PORT_VIEW_EV3_GYRO
IMG_EV3_IR_BEACON = ImageFile._PORT_VIEW_EV3_IR_BEACON
IMG_EV3_IR_BUTTON = ImageFile._PORT_VIEW_EV3_IR_BUTTON
IMG_EV3_IR_PROXIMITY = ImageFile._PORT_VIEW_EV3_IR_PROXIMITY
IMG_EV3_MOTOR_LARGE = ImageFile._PORT_VIEW_EV3_MOTOR_LARGE
IMG_EV3_MOTOR_MEDIUM = ImageFile._PORT_VIEW_EV3_MOTOR_MEDIUM
IMG_EV3_TOUCH = ImageFile._PORT_VIEW_EV3_TOUCH
IMG_EV3_ULTRASONIC = ImageFile._PORT_VIEW_EV3_ULTRASONIC
IMG_NXT_COLOR_AMBIENT = ImageFile._PORT_VIEW_NXT_COLOR_AMBIENT
IMG_NXT_COLOR_COLOR = ImageFile._PORT_VIEW_NXT_COLOR_COLOR
IMG_NXT_COLOR_REFLECTION = ImageFile._PORT_VIEW_NXT_COLOR_REFLECTION
IMG_NXT_LIGHT_AMBIENT = ImageFile._PORT_VIEW_NXT_LIGHT_AMBIENT
IMG_NXT_LIGHT_REFLECTION = ImageFile._PORT_VIEW_NXT_LIGHT_REFLECTION
IMG_NXT_SOUND = ImageFile._PORT_VIEW_NXT_SOUND
PORT_NONE_TOP = ImageFile._PORT_VIEW_P0_TOP
PORT_NONE_BOTTOM = ImageFile._PORT_VIEW_P0_BOTTOM
PORT_IMG = [
    ImageFile._PORT_VIEW_PA,
    ImageFile._PORT_VIEW_PB,
    ImageFile._PORT_VIEW_PC,
    ImageFile._PORT_VIEW_PD,
    ImageFile._PORT_VIEW_P1,
    ImageFile._PORT_VIEW_P2,
    ImageFile._PORT_VIEW_P3,
    ImageFile._PORT_VIEW_P4,
]

# Initialize brick for display and buttons.
ev3 = EV3Brick()

# We navigate through the ports with directional buttons.
# Center button increments (and wraps) mode.
PORTS = [Port.A, Port.B, Port.C, Port.D, Port.S1, Port.S2, Port.S3, Port.S4]
modes = [0] * len(PORTS)
selected = 0


# Generator that repeatedly tests which device is plugged in, reading
# values until it is unplugged, then trying again.
def port_process(index):
    while True:
        # Reset mode on plugged in.
        modes[index] = 0

        # Large and Medium Motor
        try:
            motor = Motor(PORTS[index], reset_angle=False)
            image = (
                IMG_EV3_MOTOR_LARGE
                if motor.control.pid()[0] > 12000
                else IMG_EV3_MOTOR_MEDIUM
            )
            while True:
                try:
                    short = motor.angle()
                    detail = str(short) + " deg"
                    yield image, short, detail
                except OSError:
                    motor.close()
                    break
        except OSError:
            pass

        # EV3 Touch Sensor
        try:
            sensor = EV3TouchSensor(PORTS[index])
            while True:
                try:
                    pressed = sensor.pressed()
                    detail = "pressed" if pressed else "released"
                    yield IMG_EV3_TOUCH, pressed, detail
                except OSError:
                    break
        except OSError:
            pass

        # EV3 Ultrasonic Sensor
        try:
            sensor = UltrasonicSensor(PORTS[index])
            while True:
                try:
                    distance = sensor.distance()
                    detail = f"{distance:>4} mm"
                    yield IMG_EV3_ULTRASONIC, distance, detail
                except OSError:
                    break
        except OSError:
            pass

        # EV3 Color Sensor
        try:
            sensor = EV3ColorSensor(PORTS[index])
            while True:
                try:
                    mode = modes[index] % 3
                    if mode == 2:
                        detail = str(sensor.color())[6:].lower()
                        short = detail[0:3]
                        image = IMG_EV3_COLOR_COLOR
                    else:
                        image = (
                            IMG_EV3_COLOR_AMBIENT if mode else IMG_EV3_COLOR_REFLECTION
                        )
                        short = sensor.ambient() if mode else sensor.reflection()
                        detail = f"{short:>3} %"
                    yield image, short, detail
                except OSError:
                    break
        except OSError:
            pass

        # EV3 Infrared Sensor
        try:
            sensor = InfraredSensor(PORTS[index])
            while True:
                try:
                    mode = modes[index] % 3
                    if mode == 0:
                        short = sensor.distance()
                        detail = f"{short:>3} %"
                        image = IMG_EV3_IR_PROXIMITY
                    elif mode == 1:
                        image = IMG_EV3_IR_BUTTON
                        pressed = [str(b)[7:] for b in sensor.buttons(1)]
                        short = len(pressed)
                        if len(pressed) == 1:
                            detail = pressed[0]
                        elif len(pressed) == 2:
                            detail = pressed[0] + "\n" + pressed[1]
                        else:
                            detail = "Channel 1\nNo buttons"
                    else:
                        image = IMG_EV3_IR_BEACON
                        distance, angle = sensor.beacon(1)
                        if distance is None or angle is None:
                            short = "Non"
                            detail = "Channel 1\nNo beacon"
                        else:
                            short = distance or 0
                            detail = f"{distance} %\n{angle} deg"
                    yield image, short, detail
                except OSError:
                    break
        except OSError:
            pass

        # EV3 Gyro Sensor
        try:
            gyro = GyroSensor(PORTS[index])
            while True:
                try:
                    short = gyro.angle()
                    detail = f"{short:>4} deg\n{gyro.speed():>4} deg/s"
                    yield IMG_EV3_GYRO, short, detail
                except OSError:
                    break
        except OSError:
            pass

        # NXT Color Sensor
        try:
            sensor = NXTColorSensor(PORTS[index])
            while True:
                try:
                    mode = modes[index] % 3
                    if mode == 2:
                        detail = str(sensor.color())[6:].lower()
                        short = detail[0:3]
                        image = IMG_NXT_COLOR_COLOR
                    else:
                        image = (
                            IMG_NXT_COLOR_AMBIENT if mode else IMG_NXT_COLOR_REFLECTION
                        )
                        value = sensor.ambient() if mode else sensor.reflection()
                        detail = f"{value:>5.1f} %"
                    yield image, round(value), detail
                except OSError:
                    break
        except OSError:
            pass

        # NXT Light Sensor
        try:
            sensor = LightSensor(PORTS[index])
            while True:
                try:
                    ambient = modes[index] % 2
                    image = (
                        IMG_NXT_LIGHT_AMBIENT if ambient else IMG_NXT_LIGHT_REFLECTION
                    )
                    value = sensor.ambient() if ambient else sensor.reflection()
                    detail = f"{value:>5.1f} %"
                    yield image, round(value), detail
                except OSError:
                    break
        except OSError:
            pass

        # NXT Sound Sensor
        try:
            sensor = SoundSensor(PORTS[index])
            while True:
                try:
                    value = sensor.intensity()
                    detail = f"{value:>5.1f} %"
                    yield IMG_NXT_SOUND, round(value), detail
                except OSError:
                    break
        except OSError:
            pass

        # Nothing on this port.
        yield None, "---", "No device"


# Each port has its own process that we draw values from.
processes = [port_process(i) for i in range(len(PORTS))]


def draw_ui():
    # Draw main boxes.
    ev3.screen.clear()
    ev3.screen.draw_image(0, 7, PORT_IMG[selected] if selected <= 3 else PORT_NONE_TOP)
    ev3.screen.draw_image(0, 33, ImageFile._PORT_VIEW_EMPTY)
    ev3.screen.draw_image(
        0, 92, PORT_IMG[selected] if selected > 3 else PORT_NONE_BOTTOM
    )
    ev3.screen.draw_line(50, 42, 50, 82)

    # Draw all port states.
    for i in range(len(PORTS)):
        # Get the state of this port.
        image, value, detail = next(processes[i])

        if i == selected:
            # For selection, show main image and larger value.
            if image:
                ev3.screen.draw_image(4, 40, image)
            y = 45 if "\n" in detail or "%" in detail else 55
            ev3.screen.draw_text(60, y, text=detail)
            if "%" in detail and "\n" not in detail:
                ev3.screen.draw_box(60, 65, 160, 75)
                ev3.screen.draw_box(60, 65, 60 + value, 75, fill=True)
        else:
            # Otherwise show value only.
            x = 15 + (i % 4) * 40
            y = 11 if i <= 3 else 100

            # Move integers slightly right and add optional short minus.
            if isinstance(value, int):
                x += 3
                if value < 0:
                    ev3.screen.draw_line(x - 5, y + 7, x - 2, y + 7)
                    value = -value
                if value > 999:
                    value = 999
            elif isinstance(value, str):
                value = value[0:3]

            if isinstance(value, bool):
                # Visualize bool as empty or filled box
                ev3.screen.draw_box(x + 7, y + 1, x + 17, y + 11, fill=value)
            else:
                # Else draw text value in the small box.
                ev3.screen.draw_text(x, y, text=str(value))


# Monitor the buttons to change ports, refreshing the
# screen while we wait.
while True:
    while not (pressed := ev3.buttons.pressed()):
        draw_ui()
        wait(100)

    if Button.LEFT in pressed:
        selected = (selected - 1) % len(PORTS)
    elif Button.RIGHT in pressed:
        selected = (selected + 1) % len(PORTS)
    elif Button.UP in pressed and selected > 3:
        selected -= 4
    elif Button.DOWN in pressed and selected <= 3:
        selected += 4
    elif Button.CENTER in pressed:
        modes[selected] += 1

    while any(ev3.buttons.pressed()):
        draw_ui()
        wait(100)
