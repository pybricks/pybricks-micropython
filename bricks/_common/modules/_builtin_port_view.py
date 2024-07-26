from pybricks.hubs import ThisHub
from pybricks.pupdevices import (
    DCMotor,
    Motor,
    ColorSensor,
    UltrasonicSensor,
    ForceSensor,
    ColorDistanceSensor,
    TiltSensor,
    InfraredSensor,
)
from pybricks.parameters import Port
from pybricks.tools import wait, AppData
from pybricks.iodevices import PUPDevice


# Figure out the available ports for the given hub.
ports = [Port.A, Port.B]
try:
    ports += [Port.C, Port.D]
    ports += [Port.E, Port.F]
except AttributeError:
    pass


# Allocates small buffer so the IDE can send us mode index
# values for each sensor.
app_data = AppData("b" * len(ports))

# This is sent when a device is plugged in if it has multiple modes.
# This populates a dropdown menu in the IDE to select the mode.
def make_mode_message(port, type_id, modes):
    return f"{port}\t{type_id}\tmodes\t" + "\t".join(modes) + "\r\n"


# BOOST Color and Distance Sensor
def update_color_and_distance_sensor(port, type_id, mode):
    sensor = ColorDistanceSensor(port)
    mode_info = make_mode_message(
        port,
        type_id,
        ["Reflected light intensity and color", "Ambient light intensity", "Distance"],
    )
    while True:
        if mode == 0:
            hsv = sensor.hsv()
            intensity = sensor.reflection()
            data = f"h={hsv.h}°, s={hsv.s}%, v={hsv.v}%, i={intensity}%"
        elif mode == 1:
            data = f"a={sensor.ambient()}%"
        else:
            data = f"d={sensor.distance()}%"
        yield mode_info + f"{port}\t{type_id}\t{data}"
        mode_info = ""


# SPIKE Prime / MINDSTORMS Robot Inventor Color Sensor
def update_color_sensor(port, type_id, mode):
    sensor = ColorSensor(port)
    mode_info = make_mode_message(
        port,
        type_id,
        [
            "Reflected light intensity and color",
            "Ambient light intensity and color",
        ],
    )
    while True:
        hsv = sensor.hsv(False if mode else True)
        intensity = sensor.ambient() if mode else sensor.reflection()
        yield mode_info + f"{port}\t{type_id}\th={hsv.h}°, s={hsv.s}%, v={hsv.v}%, i={intensity}%"
        mode_info = ""


# WeDo 2.0 Tilt Sensor
def update_tilt_sensor(port, type_id):
    sensor = TiltSensor(port)
    while True:
        pitch, roll = sensor.tilt()
        yield f"{port}\t{type_id}\tp={pitch}°, r={roll}°"


# WeDo 2.0 Infrared Sensor
def update_infrared_sensor(port, type_id):
    sensor = InfraredSensor(port)
    while True:
        dist = sensor.distance()
        ref = sensor.reflection()
        yield f"{port}\t{type_id}\td={dist}%, i={ref}%"


# SPIKE Prime / MINDSTORMS Robot Inventor Ultrasonic Sensor
def update_ultrasonic_sensor(port, type_id):
    sensor = UltrasonicSensor(port)
    while True:
        yield f"{port}\t{type_id}\t{sensor.distance()}mm"


# SPIKE Prime Force Sensor
def update_force_sensor(port, type_id):
    sensor = ForceSensor(port)
    while True:
        yield f"{port}\t{type_id}\t{sensor.force()}N,\t{sensor.distance()}mm"


# Any motor with rotation sensors.
def update_motor(port, type_id):
    motor = Motor(port)
    while True:
        angle = motor.angle()
        angle_mod = motor.angle() % 360
        if angle_mod > 180:
            angle_mod -= 360
        rotations = round((angle - angle_mod) / 360)
        msg = f"{port}\t{type_id}\t{motor.angle()}°"
        yield msg if angle == angle_mod else msg + f" ({rotations}R, {angle_mod}°)"


# Any motor without rotation sensors.
def update_dc_motor(port, type_id):
    while True:
        motor = DCMotor(port)
        yield f"{port}\t{type_id}"


# Any unknown Powered Up device.
def unknown_pup_device(port, type_id, dev):
    while True:
        yield f"{port}\t{type_id}\tunknown"


def device_task(index):
    port = ports[index]
    while True:
        try:
            dev = PUPDevice(port)
            type_id = dev.info()["id"]
            mode = app_data.get_values()[index]
            if type_id == 34:
                yield from update_tilt_sensor(port, type_id)
            if type_id == 35:
                yield from update_infrared_sensor(port, type_id)
            if type_id == 37:
                yield from update_color_and_distance_sensor(port, type_id, mode)
            elif type_id == 61:
                yield from update_color_sensor(port, type_id, mode)
            elif type_id == 62:
                yield from update_ultrasonic_sensor(port, type_id)
            elif type_id == 63:
                yield from update_force_sensor(port, type_id)
            elif type_id in (1, 2):
                yield from update_dc_motor(port, type_id)
            elif type_id in (38, 46, 47, 48, 49, 65, 75, 76):
                yield from update_motor(port, type_id)
            else:
                yield from unknown_pup_device(port, type_id, dev)
        except OSError:
            yield f"{port}\t--"


tasks = [device_task(index) for index in range(len(ports))]

MAX_LEN = 158

while True:
    appended = ""

    # Get the message for each sensor.
    for task in tasks:
        msg = next(task)

        # Try to concatenate the messages for reduced traffic,
        # but send early if the next message can't be appended.
        if len(appended) + len(msg) > MAX_LEN - 2:
            app_data.write_bytes(appended)
            appended = ""
        appended += msg + "\r\n"

    # Send any remaining appended data.
    app_data.write_bytes(appended)

    # Loop time.
    wait(100)
