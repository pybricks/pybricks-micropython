from pybricks.pupdevices import Motor, ColorSensor
from pybricks.tools import wait, StopWatch
from pybricks.parameters import Port, Direction
from pybricks.robotics import DriveBase
from pybricks import version

print(version)

# Initialize default "Driving Base" with medium motors and wheels.
left_motor = Motor(Port.A, Direction.COUNTERCLOCKWISE)
right_motor = Motor(Port.B)
drive_base = DriveBase(left_motor, right_motor, wheel_diameter=56, axle_track=112)

# Allocate logs for motors and controller signals.
DURATION = 6000
left_motor.log.start(DURATION)
right_motor.log.start(DURATION)
drive_base.distance_control.log.start(DURATION)
drive_base.heading_control.log.start(DURATION)
sensor = ColorSensor(Port.B)

# Line and drive constants.
BLACK = 7
WHITE = 48
threshold = (BLACK + WHITE) / 2
SPEED = 150

# Run for the duration of the logger.
watch = StopWatch()
while watch.time() < DURATION + 500:
    # Get error from the line.
    err = sensor.reflection() - threshold

    # Set steering proportional to reflection error.
    steering = err * 3
    drive_base.drive(SPEED, steering)

    wait(5)

# Stop while we print out the logs.
drive_base.stop()

# Transfer data logs.
print("Transferring data...")
left_motor.log.save("servo_left.txt")
right_motor.log.save("servo_right.txt")
drive_base.distance_control.log.save("control_distance.txt")
drive_base.heading_control.log.save("control_heading.txt")
print("Done")
