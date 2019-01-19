#!/usr/bin/env pybricks-micropython
from pybricks import ev3brick as brick
from pybricks.ev3devices import Motor, TouchSensor, ColorSensor, InfraredSensor, UltrasonicSensor, GyroSensor
from pybricks.parameters import Port, Stop, Direction, Button, Color, Image, Align, Sound
from pybricks.tools import print, wait, StopWatch

# Set up the motors
gripper = Motor(Port.A)
elbow = Motor(Port.B, Direction.counterclockwise, [8, 40])
turntable = Motor(Port.C, Direction.counterclockwise, [12, 36])
speed = 40

# Set up the sensors
touch = TouchSensor(Port.S1)
light = ColorSensor(Port.S3)

# Initialize the elbow
elbow.run_time(-speed, 1000)
elbow.run(speed)
while light.reflection() < 30:
    wait(10)
elbow.reset_angle()
elbow.stop(Stop.hold)

# Initialize the base
turntable.run(-speed)
while not touch.pressed():
    wait(10)
turntable.reset_angle()

# Initialize the gripper
gripper.run_until_stalled(150)
gripper.reset_angle()
gripper.run_target(150, -90)


###############################
# Pick up and release routines


def robot_pick(position):
    turntable.run_target(speed, position, Stop.hold)  # Move to pick up position
    elbow.run_target(speed, -40)  # Lower the arm
    gripper.run_until_stalled(150, Stop.hold)  # Close the gripper
    elbow.run_target(speed, 0, Stop.hold)  # Raise the arm


def robot_release(position):
    turntable.run_target(speed, position, Stop.hold)  # Move to release position
    elbow.run_target(speed, -40)  # Lower the arm
    gripper.run_target(150, -90)  # Open the gripper
    elbow.run_target(speed, 0, Stop.hold)  # Raise the arm


###############################
# Main script

brick.sound.beeps(3)

left, mid, right = 40, 100, 160

# Keep moving the object
while True:
    # Move object from left to middle
    robot_pick(left)
    robot_release(mid)

    # Move object from right to left
    robot_pick(right)
    robot_release(left)

    # Move object from middle to right
    robot_pick(mid)
    robot_release(right)
